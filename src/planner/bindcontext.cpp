
#include "planner/bindcontext.hpp"

#include "parser/expression/columnref_expression.hpp"

using namespace duckdb;
using namespace std;

string BindContext::GetMatchingTable(const string &column_name) {
	string result;
	if (expression_alias_map.find(column_name) != expression_alias_map.end())
		return string();
	for (auto &kv : regular_table_alias_map) {
		auto table = kv.second.table;
		if (table->ColumnExists(column_name)) {
			if (!result.empty()) {
				throw BinderException(
				    "Ambiguous reference to column name\"%s\" (use: \"%s.%s\" "
				    "or \"%s.%s\")",
				    column_name.c_str(), result.c_str(), column_name.c_str(),
				    table->name.c_str(), column_name.c_str());
			}
			result = kv.first;
		}
	}
	for (auto &kv : subquery_alias_map) {
		auto subquery = kv.second;
		throw BinderException("Subquery binding not implemented yet!");
	}

	if (result.empty()) {
		if (parent) {
			return parent->GetMatchingTable(column_name);
		}
		throw BinderException(
		    "Referenced column \"%s\" not found in FROM clause!",
		    column_name.c_str());
	}
	return result;
}

shared_ptr<ColumnCatalogEntry>
BindContext::BindColumn(ColumnRefExpression &expr, size_t depth) {
	if (expr.table_name.empty()) {
		auto entry = expression_alias_map.find(expr.column_name);
		if (entry == expression_alias_map.end()) {
			throw BinderException("Could not bind alias \"%s\"!",
			                      expr.column_name.c_str());
		}
		expr.index = entry->second.first;
		expr.reference = entry->second.second;
		expr.return_type = entry->second.second->return_type;
		return nullptr;
	}

	if (!HasAlias(expr.table_name)) {
		if (parent) {
			return parent->BindColumn(expr, ++depth);
		}
		throw BinderException("Referenced table \"%s\" not found!",
		                      expr.table_name.c_str());
	}
	shared_ptr<ColumnCatalogEntry> entry;

	size_t table_index = 0;
	auto table_entry = regular_table_alias_map.find(expr.table_name);
	if (table_entry != regular_table_alias_map.end()) {
		// base table
		auto &table = table_entry->second;
		if (!table.table->ColumnExists(expr.column_name)) {
			throw BinderException(
			    "Table \"%s\" does not have a column named \"%s\"",
			    expr.table_name.c_str(), expr.column_name.c_str());
		}
		table_index = table.index;
		entry = table.table->GetColumn(expr.column_name);
		expr.stats = table.table->GetStatistics(entry->oid);
	} else {
		// subquery
		throw BinderException("Subquery binding not implemented yet!");
	}

	auto &column_list = bound_columns[expr.table_name];
	// check if the entry already exists in the column list for the table
	expr.index = (size_t)-1;
	for (size_t i = 0; i < column_list.size(); i++) {
		auto &column = column_list[i];
		if (column == expr.column_name) {
			expr.index = i;
			break;
		}
	}
	if (expr.index == (size_t)-1) {
		expr.index = column_list.size();
		column_list.push_back(expr.column_name);
	}
	expr.table_index = table_index;
	expr.depth = depth;
	expr.return_type = entry->type;
	return entry;
}

void BindContext::GenerateAllColumnExpressions(
    vector<unique_ptr<AbstractExpression>> &new_select_list) {
	if (regular_table_alias_map.size() == 0 && subquery_alias_map.size() == 0) {
		throw BinderException("SELECT * expression without FROM clause!");
	}
	for (auto &kv : regular_table_alias_map) {
		auto table = kv.second.table;
		string table_name = table->name;
		for (auto &column : table->columns) {
			string column_name = column->name;
			new_select_list.push_back(
			    make_unique<ColumnRefExpression>(column_name, table_name));
		}
	}
	for (auto &kv : subquery_alias_map) {
		auto subquery = kv.second;
		throw BinderException("Subquery binding not implemented yet!");
	}
}

void BindContext::AddBaseTable(const string &alias,
                               shared_ptr<TableCatalogEntry> table_entry) {
	if (HasAlias(alias)) {
		throw BinderException("Duplicate alias \"%s\" in query!",
		                      alias.c_str());
	}
	regular_table_alias_map.insert(
	    make_pair(alias, TableBinding(table_entry, bound_tables++)));
}

void BindContext::AddSubquery(const string &alias, SelectStatement *subquery) {
	if (HasAlias(alias)) {
		throw BinderException("Duplicate alias \"%s\" in query!",
		                      alias.c_str());
	}
	subquery_alias_map.insert(make_pair(alias, subquery));
}

void BindContext::AddExpression(const string &alias,
                                AbstractExpression *expression, size_t i) {
	expression_alias_map[alias] = make_pair(i, expression);
}

bool BindContext::HasAlias(const string &alias) {
	return regular_table_alias_map.find(alias) !=
	           regular_table_alias_map.end() ||
	       subquery_alias_map.find(alias) != subquery_alias_map.end();
}