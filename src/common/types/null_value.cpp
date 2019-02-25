#include "common/types/null_value.hpp"

#include "common/exception.hpp"

using namespace std;

namespace duckdb {

bool IsNullValue(uint8_t *ptr, TypeId type) {
	uint8_t data[100];
	SetNullValue(data, type);
	return memcmp(ptr, data, GetTypeIdSize(type)) == 0;
}

//! Writes NullValue<T> value of a specific type to a memory address
void SetNullValue(uint8_t *ptr, TypeId type) {
	switch (type) {
	case TypeId::BOOLEAN:
	case TypeId::TINYINT:
		*((int8_t *)ptr) = NullValue<int8_t>();
		break;
	case TypeId::SMALLINT:
		*((int16_t *)ptr) = NullValue<int16_t>();
		break;
	case TypeId::INTEGER:
		*((int32_t *)ptr) = NullValue<int32_t>();
		break;
	case TypeId::DATE:
		*((date_t *)ptr) = NullValue<date_t>();
		break;
	case TypeId::BIGINT:
		*((int64_t *)ptr) = NullValue<int64_t>();
		break;
	case TypeId::TIMESTAMP:
		*((timestamp_t *)ptr) = NullValue<timestamp_t>();
		break;
	case TypeId::DECIMAL:
		*((double *)ptr) = NullValue<double>();
		break;
	case TypeId::VARCHAR:
		*((const char **)ptr) = NullValue<const char *>();
		break;
	default:
		throw InvalidTypeException(type, "Unsupported type for SetNullValue!");
	}
}

} // namespace duckdb