
#include "duckdb/common/vector.hpp"
#include "duckdb/common/serializer.hpp"
#include "duckdb/common/serializer.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"
#include "duckdb/parser/tableref/tablemacroref.hpp"

namespace duckdb {

TableMacroRef::TableMacroRef(): TableRef(TableReferenceType::TABLE_MACRO) {;
}

TableMacroRef::TableMacroRef(unique_ptr<ParsedExpression> function_p, vector<string> column_name_alias_p, string alias, unique_ptr<SampleOptions> sample_p, id_t query_location_p  )
    : TableRef(TableReferenceType::TABLE_MACRO) , function( move(function_p)), column_name_alias(column_name_alias_p) {

	this->alias=alias;

	if (sample_p) {
		this->sample=move(sample_p);
	}
	this->query_location=query_location_p;

}

TableMacroRef::TableMacroRef(unique_ptr<ParsedExpression> function_p, vector<string> column_name_alias_p)
    : TableRef(TableReferenceType::TABLE_MACRO) {
}


TableMacroRef::TableMacroRef( TableRef &ref  ) : TableRef(TableReferenceType::TABLE_MACRO) {
	this->alias = ref.alias;
	this->query_location = ref.query_location;
	this->sample = ref.sample ? ref.sample->Copy() : nullptr;
}




string TableMacroRef::ToString() const {
	return function->ToString();
}

bool TableMacroRef::Equals(const TableRef *other_p) const {
	if (!TableRef::Equals(other_p)) {
		return false;
	}
	auto other = (TableMacroRef *)other_p;
	return function->Equals(other->function.get());
}

void TableMacroRef::Serialize(Serializer &serializer) {
	TableRef::Serialize(serializer);
	function->Serialize(serializer);
	serializer.WriteString(alias);
	serializer.WriteStringVector(column_name_alias);
}

unique_ptr<TableRef> TableMacroRef::Deserialize(Deserializer &source) {
	auto result = make_unique<TableMacroRef>();

	result->function = ParsedExpression::Deserialize(source);
	result->alias = source.Read<string>();
	source.ReadStringVector(result->column_name_alias);

	return move(result);
}

unique_ptr<TableRef> TableMacroRef::Copy() {
	auto copy = make_unique<TableMacroRef>();

	copy->function = function->Copy();
	copy->column_name_alias = column_name_alias;
	CopyProperties(*copy);

	return move(copy);
}

} // namespace duckdb
