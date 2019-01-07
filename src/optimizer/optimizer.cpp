#include "optimizer/optimizer.hpp"

#include "optimizer/join_order_optimizer.hpp"
#include "optimizer/rule/list.hpp"
#include "optimizer/subquery_rewriter.hpp"
#include "planner/operator/list.hpp"

using namespace duckdb;
using namespace std;

Optimizer::Optimizer(BindContext &context) : context(context) {
	rewriter.rules.push_back(make_unique<ConstantCastRule>());
	rewriter.rules.push_back(make_unique<ConstantFoldingRule>());
	rewriter.rules.push_back(make_unique<DistributivityRule>());
	// rewriter.rules.push_back(make_unique<RemoveObsoleteFilterRule>());

#ifdef DEBUG
	for (auto &rule : rewriter.rules) {
		// root not defined in rule
		assert(rule->root);
	}
#endif
}

class OptimizeSubqueries : public LogicalOperatorVisitor {
public:
	using LogicalOperatorVisitor::Visit;
	unique_ptr<Expression> Visit(SubqueryExpression &subquery) override {
		// we perform join reordering within the subquery expression
		JoinOrderOptimizer optimizer;
		subquery.op = optimizer.Optimize(move(subquery.op));
		return nullptr;
	}
};

unique_ptr<LogicalOperator> Optimizer::Optimize(unique_ptr<LogicalOperator> plan) {
	// first we perform expression rewrites using the ExpressionRewriter
	// this does not change the logical plan structure, but only simplifies the expression trees
	rewriter.Apply(*plan);
	// then we perform the join ordering optimization
	// this also rewrites cross products + filters into joins and performs filter pushdowns
	JoinOrderOptimizer optimizer;
	auto join_order = optimizer.Optimize(move(plan));
	// perform join order optimization in subqueries as well
	OptimizeSubqueries opt;
	join_order->Accept(&opt);
	// finally we rewrite subqueries
	SubqueryRewriter subquery_rewriter(context);
	return subquery_rewriter.Rewrite(move(join_order));
}