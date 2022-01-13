#include "duckdb/catalog/catalog_entry/macro_catalog_entry.hpp"
#include "duckdb/parser/expression/function_expression.hpp"
#include "duckdb/parser/expression/subquery_expression.hpp"
#include "duckdb/parser/parsed_expression_iterator.hpp"
#include "duckdb/planner/expression_binder.hpp"
#include "duckdb/common/string_util.hpp"

namespace duckdb {

void ExpressionBinder::ReplaceMacroParametersRecursive(unique_ptr<ParsedExpression> &expr) {
	switch (expr->GetExpressionClass()) {
	case ExpressionClass::COLUMN_REF: {
		// if expr is a parameter, replace it with its argument
		auto &colref = (ColumnRefExpression &)*expr;
		bool bind_macro_parameter = false;
		if (colref.IsQualified()) {
			bind_macro_parameter = colref.GetTableName() == MacroBinding::MACRO_NAME;
		} else {
			bind_macro_parameter = macro_binding->HasMatchingBinding(colref.GetColumnName());
		}
		if (bind_macro_parameter) {
			D_ASSERT(macro_binding->HasMatchingBinding(colref.GetColumnName()));
			expr = macro_binding->ParamToArg(colref);
		}
		return;
	}
	case ExpressionClass::SUBQUERY: {
		// replacing parameters within a subquery is slightly different
		auto &sq = ((SubqueryExpression &)*expr).subquery;
		ParsedExpressionIterator::EnumerateQueryNodeChildren(
		    *sq->node, [&](unique_ptr<ParsedExpression> &child) { ReplaceMacroParametersRecursive(child); });
		break;
	}
	default: // fall through
		break;
	}
	// unfold child expressions
	ParsedExpressionIterator::EnumerateChildren(
	    *expr, [&](unique_ptr<ParsedExpression> &child) { ReplaceMacroParametersRecursive(child); });
}

BindResult ExpressionBinder::BindMacro(FunctionExpression &function, MacroCatalogEntry *macro_func, idx_t depth,
                                       unique_ptr<ParsedExpression> *expr) {
	auto &macro_def = *macro_func->function;
	// validate the arguments and separate positional and default arguments
	vector<unique_ptr<ParsedExpression>> positionals;
	unordered_map<string, unique_ptr<ParsedExpression>> defaults;

	if (macro_func->function->is_query()) {
		auto error = StringUtil::Format("Select Macro %s is being used in the wrong context as a regular macro\n Try "
		                                "running again with the command SELECT FUNCTION %s\n",
		                                function.function_name, function.ToString());
		return BindResult(binder.FormatError(*expr->get(), error));
	}

	string error = MacroFunction::ValidateArguments(*macro_func, function, positionals, defaults);
	if (!error.empty()) {
		return BindResult(binder.FormatError(*expr->get(), error));
	}

	// create a MacroBinding to bind this macro's parameters to its arguments
	vector<LogicalType> types;
	vector<string> names;
	// positional parameters
	for (idx_t i = 0; i < macro_def.parameters.size(); i++) {
		types.emplace_back(LogicalType::SQLNULL);
		auto &param = (ColumnRefExpression &)*macro_def.parameters[i];
		names.push_back(param.GetColumnName());
	}
	// default parameters
	for (auto it = macro_def.default_parameters.begin(); it != macro_def.default_parameters.end(); it++) {
		types.emplace_back(LogicalType::SQLNULL);
		names.push_back(it->first);
		// now push the defaults into the positionals
		positionals.push_back(move(defaults[it->first]));
	}
	auto new_macro_binding = make_unique<MacroBinding>(types, names, macro_func->name);
	new_macro_binding->arguments = move(positionals);
	macro_binding = new_macro_binding.get();

	// replace current expression with stored macro expression, and replace params
	*expr = macro_func->function->expression->Copy();
	ReplaceMacroParametersRecursive(*expr);

	// bind the unfolded macro
	return BindExpression(expr, depth);
}

} // namespace duckdb
