#include "duckdb/parser/tableref/subqueryref.hpp"
#include "duckdb/planner/binder.hpp"
#include "duckdb/planner/tableref/bound_subqueryref.hpp"


namespace duckdb {


unique_ptr<BoundTableRef> Binder::Bind(TableMacroRef &ref) {
	//QueryErrorContext error_context(root_statement, ref.query_location);
	//auto bind_index = GenerateTableIndex();

	/*
	D_ASSERT(ref.function->type == ExpressionType::FUNCTION);
	auto fexpr = (FunctionExpression *)ref.function.get();

	// evaluate the input parameters to the function
	vector<LogicalType> arguments;
	vector<Value> parameters;
	named_parameter_map_t named_parameters;
	unique_ptr<BoundSubqueryRef> subquery;
	string error;
	if (!BindFunctionParameters(fexpr->children, arguments, parameters, named_parameters, subquery, error)) {
		throw BinderException(FormatError(ref, error));
	}

	// fetch the function from the catalog
	auto &catalog = Catalog::GetCatalog(context);
	auto function =
	    catalog.GetEntry<TableFunctionCatalogEntry>(context, fexpr->schema, fexpr->function_name, false, error_context);

	// select the function based on the input parameters
	idx_t best_function_idx = Function::BindFunction(function->name, function->functions, arguments, error);
	if (best_function_idx == DConstants::INVALID_INDEX) {
		throw BinderException(FormatError(ref, error));
	}
	auto &table_function = function->functions[best_function_idx];

	*/
     return nullptr;

} // namespace duckdb
