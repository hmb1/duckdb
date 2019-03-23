#include "optimizer/rule/constant_folding.hpp"

#include "common/exception.hpp"
#include "execution/expression_executor.hpp"
#include "optimizer/expression_rewriter.hpp"

using namespace duckdb;
using namespace std;

//! The ScalarExpressionMatcher matches on any scalar expression (i.e. Expression::IsScalar is true)
class ConstantFoldingExpressionMatcher : public FoldableConstantMatcher {
public:
	bool Match(Expression *expr, vector<Expression *> &bindings) override {
		// we also do not match on ConstantExpressions, because we cannot fold those any further
		if (expr->type == ExpressionType::VALUE_CONSTANT) {
			return false;
		}
		return FoldableConstantMatcher::Match(expr, bindings);
	}
};

ConstantFoldingRule::ConstantFoldingRule(ExpressionRewriter &rewriter) : Rule(rewriter) {
	auto op = make_unique<ConstantFoldingExpressionMatcher>();
	root = move(op);
}

unique_ptr<Expression> ConstantFoldingRule::Apply(LogicalOperator &op, vector<Expression *> &bindings,
                                                  bool &changes_made) {
	auto root = bindings[0];
	// the root is a scalar expression that we have to fold
	assert(root->IsScalar() && root->type != ExpressionType::VALUE_CONSTANT);

	// use an ExpressionExecutor to execute the expression
	auto result_value = ExpressionExecutor::EvaluateScalar(*root);
	// now get the value from the result vector and insert it back into the plan as a constant expression
	return make_unique<ConstantExpression>(result_value.CastAs(root->return_type));
}