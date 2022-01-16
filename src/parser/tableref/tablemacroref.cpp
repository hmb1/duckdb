#include "duckdb/parser/tableref/tablemacroref.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/common/serializer.hpp"

namespace duckdb {

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
