//===----------------------------------------------------------------------===//
//                         DuckDB
//
// planner/operator/logical_union.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "planner/logical_operator.hpp"

namespace duckdb {

class LogicalUnion : public LogicalOperator {
public:
	LogicalUnion(unique_ptr<LogicalOperator> top_select, unique_ptr<LogicalOperator> bottom_select)
	    : LogicalOperator(LogicalOperatorType::UNION) {
		AddChild(move(top_select));
		AddChild(move(bottom_select));
	}

	vector<string> GetNames() override {
		return children[0]->GetNames();
	}

protected:
	void ResolveTypes() override {
		types = children[0]->types;
	}
};
} // namespace duckdb