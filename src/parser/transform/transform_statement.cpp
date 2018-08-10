
#include "common/assert.hpp"

#include "catalog/column_catalog.hpp"

#include "parser/tableref/tableref_list.hpp"
#include "parser/transform.hpp"

using namespace std;

namespace duckdb {

bool TransformGroupBy(List *group,
                      vector<unique_ptr<AbstractExpression>> &result) {
	if (!group) {
		return false;
	}

	for (auto node = group->head; node != nullptr; node = node->next) {
		Node *n = reinterpret_cast<Node *>(node->data.ptr_value);
		result.push_back(TransformExpression(n));
	}
	return true;
}

bool TransformOrderBy(List *order, OrderByDescription &result) {
	if (!order) {
		return false;
	}

	for (auto node = order->head; node != nullptr; node = node->next) {
		Node *temp = reinterpret_cast<Node *>(node->data.ptr_value);
		if (temp->type == T_SortBy) {
			OrderByNode ordernode;
			SortBy *sort = reinterpret_cast<SortBy *>(temp);
			Node *target = sort->node;
			if (sort->sortby_dir == SORTBY_ASC ||
			    sort->sortby_dir == SORTBY_DEFAULT)
				ordernode.type = OrderType::ASCENDING;
			else if (sort->sortby_dir == SORTBY_DESC)
				ordernode.type = OrderType::DESCENDING;
			else
				throw NotImplementedException("Unimplemented order by type");
			ordernode.expression = TransformExpression(target);
			result.orders.push_back(
			    OrderByNode(ordernode.type, move(ordernode.expression)));
		} else {
			throw NotImplementedException("ORDER BY list member type %d\n",
			                              temp->type);
		}
	}
	return true;
}

unique_ptr<SelectStatement> TransformSelect(Node *node) {
	SelectStmt *stmt = reinterpret_cast<SelectStmt *>(node);
	switch (stmt->op) {
	case SETOP_NONE: {
		auto result = make_unique<SelectStatement>();
		result->select_distinct = stmt->distinctClause != NULL ? true : false;
		if (!TransformExpressionList(stmt->targetList, result->select_list)) {
			return nullptr;
		}
		result->from_table = TransformFrom(stmt->fromClause);
		TransformGroupBy(stmt->groupClause, result->groupby.groups);
		result->groupby.having = TransformExpression(stmt->havingClause);
		TransformOrderBy(stmt->sortClause, result->orderby);
		result->where_clause = TransformExpression(stmt->whereClause);
		if (stmt->limitCount) {
			result->limit.limit =
			    reinterpret_cast<A_Const *>(stmt->limitCount)->val.val.ival;
			result->limit.offset =
			    !stmt->limitOffset
			        ? 0
			        : reinterpret_cast<A_Const *>(stmt->limitOffset)
			              ->val.val.ival;
		}
		return result;
	}
	case SETOP_UNION: {
		auto result = TransformSelect((Node *)stmt->larg);
		if (!result) {
			return nullptr;
		}
		auto right = TransformSelect((Node *)stmt->rarg);
		if (!right) {
			return nullptr;
		}
		result->union_select = move(right);
		return result;
	}
	case SETOP_INTERSECT:
	case SETOP_EXCEPT:
	default:
		throw NotImplementedException("A_Expr not implemented!");
	}
}

unique_ptr<CreateStatement> TransformCreate(Node *node) {
	CreateStmt *stmt = reinterpret_cast<CreateStmt *>(node);
	assert(stmt);
	auto result = make_unique<CreateStatement>();
	if (stmt->inhRelations) {
		throw NotImplementedException("inherited relations not implemented");
	}
	RangeVar *relation;
	assert(stmt->relation);

	if (stmt->relation->schemaname) {
		result->schema = stmt->relation->schemaname;
	}
	result->table = stmt->relation->relname;
	assert(stmt->tableElts);
	ListCell *c;

	for (c = stmt->tableElts->head; c != NULL; c = lnext(c)) {
		ColumnDef *cdef = (ColumnDef *)c->data.ptr_value;
		if (cdef->type != T_ColumnDef) {
			throw NotImplementedException("Can only handle basic columns");
		}
		char *name = (reinterpret_cast<value *>(
		                  cdef->typeName->names->tail->data.ptr_value)
		                  ->val.str);
		auto centry = ColumnCatalogEntry(
		    cdef->colname, TransformStringToTypeId(name), cdef->is_not_null);
		result->columns.push_back(centry);
	}
	return result;
}

unique_ptr<InsertStatement> TransformInsert(Node *node) {
	InsertStmt *stmt = reinterpret_cast<InsertStmt *>(node);
	assert(stmt);

	auto select_stmt = reinterpret_cast<SelectStmt *>(stmt->selectStmt);
	if (select_stmt->fromClause) {
		throw NotImplementedException("Can only handle basic inserts");
	}

	auto result = make_unique<InsertStatement>();
	assert(select_stmt->valuesLists);

	if (stmt->cols) {
		for (ListCell *c = stmt->cols->head; c != NULL; c = lnext(c)) {
			ResTarget *target = (ResTarget *)(c->data.ptr_value);
			result->columns.push_back(std::string(target->name));
		}
	}

	if (!TransformValueList(select_stmt->valuesLists, result->values)) {
		throw Exception("Failed to transform value list");
	}

	auto ref = TransformRangeVar(stmt->relation);
	auto &table = *reinterpret_cast<BaseTableRef *>(ref.get());
	result->table = table.table_name;
	result->schema = table.schema_name;
	return result;
}
} // namespace duckdb