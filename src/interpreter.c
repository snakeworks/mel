#include "interpreter.h"
#include "base.h"
#include "native.h"
#include "parser.h"
#include <assert.h>
#include <string.h>

#define MAX_ARGS 32

static Value make_value_number(f64 value) {
  Value v;
  v.type = TYPE_NUMBER;
  v.as.number = value;
  return v;
}

static Value make_value_boolean(bool value) {
  Value v;
  v.type = TYPE_BOOLEAN;
  v.as.boolean = value;
  return v;
}

static Value make_value_string(StringView value) {
  Value v;
  v.type = TYPE_STRING;
  v.as.string = value;
  return v;
}

static Binding *get_binding(InterpreterContext *context, StringView identifier) {
  for (u32 i = 0; i < context->bindings->size; i++) {
    if (
      sv_is_equal(
        context->bindings->items[i].identifier,
        identifier
      )
    ) {
      return &context->bindings->items[i];
    }
  }
  return NULL;
}

static void set_binding(InterpreterContext *context, StringView identifier, Value value) {
  Binding *existing = get_binding(context, identifier);
  if (existing != NULL) {
    existing->value = value;
  } else {
    Binding binding = {
      .identifier = identifier,
      .value = value
    };
    da_append(context->bindings, binding);
  }
}

static Value eval_expr(InterpreterContext *context, Expr *expr);

static Value eval_value_subscript(InterpreterContext *context, Value value, u64 index) {
  switch (value.type) {
  case TYPE_ARRAY:
    if (index < value.as.array->size) {
      return eval_expr(context, value.as.array->items[index]);
    }
    return NULL_VALUE;
  case TYPE_NUMBER:
    if (index < value.as.number) {
      return make_value_number(index);
    }
    return NULL_VALUE;
  case TYPE_BOOLEAN:
  case TYPE_STRING:
  case TYPE_NULL:
    return NULL_VALUE;
  }
  return NULL_VALUE;
}

static Value eval_expr_subscript(InterpreterContext *context, Expr *expr, u64 index) {
  switch (expr->kind) {
  case EXPR_VALUE: return eval_value_subscript(context, expr->as.value, index);
  case EXPR_IDENTIFIER: {
    Binding *binding = get_binding(context, expr->as.identifier);
    if (binding == NULL) {
      return NULL_VALUE;
    }
    return eval_value_subscript(context, binding->value, index);
  }
  case EXPR_UNARY:
  case EXPR_BINARY:
  case EXPR_CALL:
  case EXPR_ITER:
    return NULL_VALUE;
  }
  return NULL_VALUE;
}

static Value eval_expr(InterpreterContext *context, Expr *expr) {
  if (expr == NULL) {
    return NULL_VALUE;
  }

  switch (expr->kind) {
  case EXPR_VALUE:
    return expr->as.value;
  case EXPR_IDENTIFIER: {
    Binding *binding = get_binding(context, expr->as.identifier);
    return binding->value;
  }
  case EXPR_UNARY: {
    Value v = eval_expr(context, expr->as.unary.right);
    if (expr->as.unary.op == TOK_MINUS) {
      return make_value_number(-v.as.number);
    }
    return v;
  }
  case EXPR_BINARY: {
    Value l = eval_expr(context, expr->as.binary.left);
    Value r = eval_expr(context, expr->as.binary.right);

    assert(l.type == r.type); // TODO: Will cause problems later

    switch (l.type) {
    case TYPE_NUMBER: {
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
    case TYPE_BOOLEAN:
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
    case TYPE_STRING:
      switch (expr->as.binary.op) {
      case TOK_DOUBLE_EQUAL: return make_value_boolean(sv_is_equal(l.as.string, r.as.string));
      case TOK_BANG_EQUAL: return make_value_boolean(!sv_is_equal(l.as.string, r.as.string));
      default: return NULL_VALUE;
      }
      break;
    case TYPE_ARRAY: {
      break;
    }
    case TYPE_NULL:
      // TODO: Implement
      break;
    }
    break;
  }
  case EXPR_CALL: {
    if (expr->as.call.callee->kind != EXPR_IDENTIFIER) return NULL_VALUE; // TODO: Runtime error

    NativeFn fn = map_identifier_to_native_fn(expr->as.call.callee->as.identifier);
    if (fn != NULL) {
      u32 count = expr->as.call.args->size;

      if (count > MAX_ARGS) {
        // TODO: Runtime error
        return NULL_VALUE;
      }

      Value values[MAX_ARGS];
      for (u32 i = 0; i < count; i++) {
        values[i] = eval_expr(context, expr->as.call.args->items[i]);
      }

      return fn(values, count);
    }

    return NULL_VALUE;
  }
  case EXPR_ITER: {
    Value subscript = eval_expr_subscript(context, expr->as.iter.iterable, expr->as.iter.cur_index++);
    if (subscript.type == TYPE_NULL) {
      return make_value_boolean(false);
    }
    set_binding(context, expr->as.iter.el_identifier, subscript);
    return make_value_boolean(true);
  }
  default:
    return NULL_VALUE;
  }
  return NULL_VALUE;
}

static void stmt_exec(InterpreterContext *context, Stmt stmt);
static void stmt_array_exec(InterpreterContext *context, StmtArray *array);

static bool is_truthy(Value value) {
  switch (value.type) {
  case TYPE_NUMBER:
    return value.as.number > 0.0;
  case TYPE_BOOLEAN:
    return value.as.boolean;
  case TYPE_STRING:
  case TYPE_ARRAY:
  case TYPE_NULL:
    return false;
  }
  return false;
}

static void stmt_exec(InterpreterContext *context, Stmt stmt) {
  switch (stmt.kind) {
  case STMT_EMPTY:
    break;
  case STMT_EXPR: {
    eval_expr(context, stmt.as.expr);
    break;
  }
  case STMT_BLOCK:
    stmt_array_exec(context, stmt.as.block);
    break;
  case STMT_IF: {
    Value condition = eval_expr(context, stmt.as.if_branch.condition);
    if (is_truthy(condition)) {
      stmt_exec(context, *stmt.as.if_branch.body);
    } else if (stmt.as.if_branch.else_body != NULL) {
      stmt_exec(context, *stmt.as.if_branch.else_body);
    }
    break;
  }
  case STMT_FOR: {
    if (stmt.as.for_loop.condition == NULL) {
      while (true) {
        stmt_exec(context, *stmt.as.for_loop.body);
      }
    } else {
      while (is_truthy(eval_expr(context, stmt.as.for_loop.condition))) {
        stmt_exec(context, *stmt.as.for_loop.body);
      }
    }
    break;
  }
  case STMT_ASSIGN: {
    Value assign_value = eval_expr(context, stmt.as.assign.assignment);
    set_binding(context, stmt.as.assign.identifier, assign_value);
    break;
  }
  }
}

static void stmt_array_exec(InterpreterContext *context, StmtArray *array) {
  for (u32 i = 0; i < array->size; i++) {
    stmt_exec(context, array->items[i]);
  }
}

void interpreter_begin(InterpreterResult *result, StmtArray *statements) {
  InterpreterContext context = {0};
  context.bindings = malloc(sizeof(Binding));
  context.errors = malloc(sizeof(LogArray));

  da_init(context.bindings, 8);
  da_init(context.errors, 8);

  result->exit_code = 0;
  stmt_array_exec(&context, statements);

  da_free(context.bindings);
  da_free(context.errors);
  free(context.bindings);
  free(context.errors);
}

