//===----------------------------------------------------------------------===//
//                         DuckDB
//
// planner/operator/logical_join.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "planner/logical_operator.hpp"

namespace duckdb {

struct JoinCondition {
	unique_ptr<Expression> left;
	unique_ptr<Expression> right;
	ExpressionType comparison;
};

enum JoinSide { NONE, LEFT, RIGHT, BOTH };

//! LogicalJoin represents a join between two relations
class LogicalJoin : public LogicalOperator {
public:
	LogicalJoin(JoinType type) : LogicalOperator(LogicalOperatorType::JOIN), type(type) {
	}

	void Accept(LogicalOperatorVisitor *v) override {
		v->Visit(*this);
	}
	vector<string> GetNames() override;

	vector<JoinCondition> conditions;
	JoinType type;

	string ParamsToString() const override;

protected:
	void ResolveTypes() override;
};
} // namespace duckdb