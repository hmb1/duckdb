//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/parser/tableref/table_function_ref.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/parser/parsed_expression.hpp"
#include "duckdb/parser/tableref.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"

namespace duckdb {
//! Represents a Table producing function
class TableMacroRef : public TableRef {
public:
	TableMacroRef();

	TableMacroRef(unique_ptr<ParsedExpression> function_p, vector<string> column_name_alias_p, string alias,
	              unique_ptr<SampleOptions> sample_p, id_t query_location_p);

	TableMacroRef(unique_ptr<ParsedExpression> function_p, vector<string> column_name_alias_p);

	explicit TableMacroRef(TableRef &ref);

	unique_ptr<ParsedExpression> function;
	vector<string> column_name_alias;

	// if the function takes a subquery as argument its in here
	unique_ptr<SelectStatement> subquery;

public:
	string ToString() const override;

	bool Equals(const TableRef *other_p) const override;

	unique_ptr<TableRef> Copy() override;

	//! Serializes a blob into a BaseTableRef
	void Serialize(Serializer &serializer) override;
	//! Deserializes a blob back into a BaseTableRef
	static unique_ptr<TableRef> Deserialize(Deserializer &source);
};
} // namespace duckdb
