#include "interpreter.h"
#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Value make_value_number(f64 value) {
  Value v;
  v.kind = VAL_NUMBER;
  v.as.number = value;
  return v;
}

static Value make_value_boolean(bool value) {
  Value v;
  v.kind = VAL_BOOLEAN;
  v.as.boolean = value;
  return v;
}

static Value make_value_string(StringView value) {
  Value v;
  v.kind = VAL_STRING;
  v.as.string = value;
  return v;
}

void value_print(Value value) {
  switch (value.kind) {
  case VAL_NUMBER:
    printf("%g", value.as.number);
    break;
  case VAL_BOOLEAN:
    printf(value.as.boolean ? "true" : "false");
    break;
  case VAL_STRING:
    printf(SV_FMT, SV_ARG(value.as.string));
    break;
  case VAL_NULL:
    printf("null");
    break;
  }
}

static Value eval_expr(Expr *expr) {
  if (expr == NULL) {
    return NULL_VALUE;
  }

  switch (expr->kind) {
  case EXPR_VALUE:
    return expr->as.value;
  case EXPR_UNARY: {
    Value v = eval_expr(expr->as.unary.right);
    if (expr->as.unary.op == TOK_MINUS) {
      return make_value_number(-v.as.number);
    }
    return v;
  }
  case EXPR_BINARY: {
    Value l = eval_expr(expr->as.binary.left);
    Value r = eval_expr(expr->as.binary.right);

    assert(l.kind == r.kind); // TODO: Will cause problems later

    switch (l.kind) {
    case VAL_NUMBER: {
      switch (expr->as.binary.op) {
      case TOK_PLUS: return make_value_number(l.as.number + r.as.number);
      case TOK_MINUS: return make_value_number(l.as.number - r.as.number);
      case TOK_STAR: return make_value_number(l.as.number * r.as.number);
      case TOK_SLASH: return make_value_number(l.as.number / r.as.number);
      case TOK_LESS: return make_value_boolean(l.as.number < r.as.number);
      case TOK_LESS_EQUAL: return make_value_boolean(l.as.number <= r.as.number);
      case TOK_DOUBLE_EQUAL: return make_value_boolean(l.as.number == r.as.number);
      case TOK_BANG_EQUAL: return make_value_boolean(l.as.number != r.as.number);
      case TOK_GREATER: return make_value_boolean(l.as.number > r.as.number);
      case TOK_GREATER_EQUAL: return make_value_boolean(l.as.number >= r.as.number);
      default: return make_value_number(0.0);
      }
    }
    case VAL_BOOLEAN:
      switch (expr->as.binary.op) {
      case TOK_LESS: return make_value_boolean(l.as.boolean < r.as.boolean);
      case TOK_LESS_EQUAL: return make_value_boolean(l.as.boolean <= r.as.boolean);
      case TOK_DOUBLE_EQUAL: return make_value_boolean(l.as.boolean == r.as.boolean);
      case TOK_BANG_EQUAL: return make_value_boolean(l.as.boolean != r.as.boolean);
      case TOK_GREATER: return make_value_boolean(l.as.boolean > r.as.boolean);
      case TOK_GREATER_EQUAL: return make_value_boolean(l.as.boolean >= r.as.boolean);
      default: return make_value_boolean(false);
      }
      break;
    case VAL_STRING:
      switch (expr->as.binary.op) {
      case TOK_DOUBLE_EQUAL: return make_value_boolean(sv_is_equal(l.as.string, r.as.string));
      case TOK_BANG_EQUAL: return make_value_boolean(!sv_is_equal(l.as.string, r.as.string));
      default: return NULL_VALUE;
      }
      break;
    case VAL_NULL:
      // TODO: Implement
      break;
    }
    break;
  }
  default:
    return NULL_VALUE;
  }
  return NULL_VALUE;
}

static void stmt_exec(Stmt stmt);
static void stmt_array_exec(StmtArray *array);

static bool is_truthy(Value value) {
  switch (value.kind) {
  case VAL_NUMBER:
    return value.as.number > 0.0;
  case VAL_BOOLEAN:
    return value.as.boolean;
  case VAL_STRING:
  case VAL_NULL:
    return false;
  }
  return false;
}

static void stmt_exec(Stmt stmt) {
  switch (stmt.kind) {
  case STMT_EMPTY:
    break;
  case STMT_EXPR: {
    switch (stmt.as.expr->kind) {
    case EXPR_CALL:
      if (sv_is_equal_to_cstr(
        stmt.as.expr->as.call.callee->as.identifier,
        "print"
      )) {
        Value value = eval_expr(stmt.as.expr->as.call.args->items[0]);
        value_print(value);
      }
      break;
    default:
      break;
    }
    break;
  }
  case STMT_BLOCK:
    stmt_array_exec(stmt.as.block);
    break;
  case STMT_IF: {
    Value value = eval_expr(stmt.as.if_branch.condition);
    if (is_truthy(value)) {
      stmt_exec(*stmt.as.if_branch.body);
    }
    break;
  }
  }
}

static void stmt_array_exec(StmtArray *array) {
  for (u32 i = 0; i < array->size; i++) {
    stmt_exec(array->items[i]);
  }
}

void interpreter_begin(InterpreterResult *result, StmtArray *statements) {
  result->exit_code = 0;
  stmt_array_exec(statements);
}

