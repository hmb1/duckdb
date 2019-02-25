#include "append_info.h"
#include "append_info.hpp"
#include "config.h"
#include "porting.h"
#include "nulls.h"
#include "date.h"

#include "common/exception.hpp"
#include "storage/data_table.hpp"
#include <cstring>

using namespace tpcds;

append_info *append_info_get(void *info_list, int table_id) {
	auto arr = (tpcds_append_information *)info_list;
	return (append_info *)&arr[table_id];
}

void append_row_start(append_info *info) {
	auto append_info = (tpcds_append_information *)info;
	append_info->appender->BeginRow();
}

void append_row_end(append_info *info) {
	auto append_info = (tpcds_append_information *)info;
	append_info->appender->EndRow();
}

void append_varchar(append_info *info, const char *value) {
	auto append_info = (tpcds_append_information *)info;
	if (!nullCheck(append_info->col)) {
		append_info->appender->AppendString(value);
	} else {
		append_info->appender->AppendValue(duckdb::Value());
	}
}

// TODO: use direct array manipulation for speed, but not now
static void append_value(append_info *info, duckdb::Value v) {
	auto append_info = (tpcds_append_information *)info;
	append_info->appender->AppendValue(v);
}

void append_key(append_info *info, int64_t value) {
	auto append_info = (tpcds_append_information *)info;
	append_info->appender->AppendBigInt(value);
}

void append_integer(append_info *info, int32_t value) {
	auto append_info = (tpcds_append_information *)info;
	append_info->appender->AppendInteger(value);
}

void append_boolean(append_info *info, int32_t value) {
	append_value(info, duckdb::Value::BOOLEAN((int8_t)value));
}

// value is a Julian date
// FIXME: direct int conversion, offsets should be constant
void append_date(append_info *info, int64_t value) {
	date_t dTemp;
	jtodt(&dTemp, (int)value);
	auto ddate = duckdb::Date::FromDate(dTemp.year, dTemp.month, dTemp.day);
	append_value(info, duckdb::Value::DATE(ddate));
}

void append_decimal(append_info *info, decimal_t *val) {
	double dTemp = val->number;
	for (int i = 0; i < val->precision; i++) {
		dTemp /= 10.0;
	}
	append_value(info, duckdb::Value(dTemp));
}