#include "parser/expression/default_expression.hpp"

#include "common/exception.hpp"

using namespace duckdb;
using namespace std;

unique_ptr<Expression> DefaultExpression::Copy() {
	auto copy = make_unique<DefaultExpression>();
	copy->CopyProperties(*this);
	return move(copy);
}

unique_ptr<Expression> DefaultExpression::Deserialize(ExpressionType type, TypeId return_type, Deserializer &source) {
	return make_unique<DefaultExpression>();
}