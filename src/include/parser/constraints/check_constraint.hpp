//===----------------------------------------------------------------------===//
//                         DuckDB
//
// parser/constraints/check_constraint.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/string_util.hpp"
#include "parser/constraint.hpp"
#include "parser/expression.hpp"
#include "parser/sql_node_visitor.hpp"

namespace duckdb {

//! The CheckConstraint contains an expression that must evaluate to TRUE for
//! every row in a table
class CheckConstraint : public Constraint {
public:
	CheckConstraint(unique_ptr<Expression> expression)
	    : Constraint(ConstraintType::CHECK), expression(move(expression)){};
	virtual ~CheckConstraint() {
	}

	virtual unique_ptr<Constraint> Accept(SQLNodeVisitor *v) {
		return v->Visit(*this);
	}

	virtual string ToString() const {
		return StringUtil::Format("CHECK(%s)", expression->ToString().c_str());
	}

	//! Serialize to a stand-alone binary blob
	virtual void Serialize(Serializer &serializer);
	//! Deserializes a CheckConstraint
	static unique_ptr<Constraint> Deserialize(Deserializer &source);

	unique_ptr<Expression> expression;
};

} // namespace duckdb