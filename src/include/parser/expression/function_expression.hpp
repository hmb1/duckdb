//===----------------------------------------------------------------------===//
//
//                         DuckDB
//
// parser/expression/function_expression.hpp
//
// Author: Mark Raasveldt
//
//===----------------------------------------------------------------------===//

#pragma once

#include "parser/expression/abstract_expression.hpp"

namespace duckdb {
//! Represents a function call
class FunctionExpression : public AbstractExpression {
  public:
	FunctionExpression(
	    const char *func_name,
	    std::vector<std::unique_ptr<AbstractExpression>> &children)
	    : AbstractExpression(ExpressionType::FUNCTION), func_name(func_name) {
		for (auto &child : children) {
			AddChild(std::move(child));
		}
	}

	virtual void Accept(SQLNodeVisitor *v) override { v->Visit(*this); }

  private:
	std::string func_name;
	std::vector<TypeId> func_arg_types;
};
} // namespace duckdb