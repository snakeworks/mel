#include "interpreter.h"
#include "base.h"
#include "lexer.h"
#include "native.h"
#include "parser.h"
#include <assert.h>
#include <string.h>

#define MAX_ARGS 32

static Value make_value_int(i64 value) {
  Value v;
  v.type = TYPE_INT;
  v.as.integer = value;
  return v;
}

static Value make_value_float(f64 value) {
  Value v;
  v.type = TYPE_FLOAT;
  v.as.floating_point = value;
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

static Environment *make_env(InterpreterContext *context) {
  Environment *env = malloc(sizeof(Environment));
  env->parent = context->cur_env;
  env->bindings = malloc(sizeof(BindingArray));
  da_init(env->bindings, 8);
  return env;
}

static void free_cur_env(InterpreterContext *context) {
  da_free(context->cur_env->bindings);
  free(context->cur_env->bindings);
  free(context->cur_env);
}

static Binding *get_binding_from_env(Environment *env, StringView identifier) {
  for (u32 i = 0; i < env->bindings->size; i++) {
    if (
      sv_is_equal(
        env->bindings->items[i].identifier,
        identifier
      )
    ) {
      return &env->bindings->items[i];
    }
  }
  return NULL;
}

static Binding *get_binding(InterpreterContext *context, StringView identifier) {
  Environment *env = context->cur_env;
  while (env != NULL) {
    Binding *binding = get_binding_from_env(env, identifier);
    if (binding != NULL) {
      return binding;
    }
    env = env->parent;
  }
  return NULL;
}

static void set_env_binding(Environment *env, StringView identifier, Value value) {
  Binding *existing = get_binding_from_env(env, identifier);
  if (existing != NULL) {
    existing->value = value;
  } else {
    Binding binding = {
      .identifier = identifier,
      .value = value
    };
    da_append(env->bindings, binding);
  }
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
    da_append(context->cur_env->bindings, binding);
  }
}

static Stmt *find_fn(StmtArray *stmts, StringView identifier) {
  for (u32 i = 0; i < stmts->size; i++) {
    if (
      stmts->items[i].kind == STMT_FN_DECLARE &&
      sv_is_equal(stmts->items[i].as.fn_declare.identifier, identifier)
    ) {
      return &stmts->items[i];
    }
  }
  return NULL;
}

static Value eval_expr(InterpreterContext *context, Expr *expr);
static bool is_truthy(Value value);

static Value eval_value_subscript(InterpreterContext *context, Value value, i64 index) {
  switch (value.type) {
  case TYPE_ARRAY:
    if (index < value.as.array->size) {
      return eval_expr(context, value.as.array->items[index]);
    }
    return NULL_VALUE;
  case TYPE_INT:
    if (index < value.as.integer) {
      return make_value_int(index);
    }
    return NULL_VALUE;
  case TYPE_FLOAT:
  case TYPE_BOOLEAN:
  case TYPE_STRING:
  case TYPE_NULL:
    return NULL_VALUE;
  }
  return NULL_VALUE;
}

// TODO: Index is signed because in the future we may have `array[-1]` to get elements starting from the end
static Value eval_expr_subscript(InterpreterContext *context, Expr *expr, i64 index) {
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

static void stmt_exec(InterpreterContext *context, Stmt stmt);
static void stmt_array_exec(InterpreterContext *context, StmtArray *array);

static Value eval_expr(InterpreterContext *context, Expr *expr) {
  if (expr == NULL) {
    return NULL_VALUE;
  }

  switch (expr->kind) {
  case EXPR_VALUE:
    return expr->as.value;
  case EXPR_IDENTIFIER: {
    Binding *binding = get_binding(context, expr->as.identifier);
    if (binding == NULL) {
      return NULL_VALUE;
    }
    return binding->value;
  }
  case EXPR_UNARY: {
    Value v = eval_expr(context, expr->as.unary.right);
    if (expr->as.unary.op == TOK_MINUS) {
      if (v.type == TYPE_INT) return make_value_int(-v.as.integer);
      else if (v.type == TYPE_FLOAT) return make_value_float(-v.as.floating_point);
    }
    return v;
  }
  case EXPR_BINARY: {
    Value l = eval_expr(context, expr->as.binary.left);
    Value r = eval_expr(context, expr->as.binary.right);

    switch (expr->as.binary.op) {
    case TOK_OR:  return make_value_boolean(is_truthy(l) || is_truthy(r));
    case TOK_AND: return make_value_boolean(is_truthy(l) && is_truthy(r));
    default: break;
    }

    bool l_num = (l.type == TYPE_INT || l.type == TYPE_FLOAT);
    bool r_num = (r.type == TYPE_INT || r.type == TYPE_FLOAT);

    if (l_num && r_num) {
      if (l.type == TYPE_INT && r.type == TYPE_INT) {
        i64 a = l.as.integer, b = r.as.integer;
        switch (expr->as.binary.op) {
        case TOK_PLUS: return make_value_int(a + b);
        case TOK_MINUS: return make_value_int(a - b);
        case TOK_STAR: return make_value_int(a * b);
        case TOK_SLASH: return make_value_int(a / b);  // TODO: divide by zero
        case TOK_LESS: return make_value_boolean(a <  b);
        case TOK_LESS_EQUAL: return make_value_boolean(a <= b);
        case TOK_GREATER: return make_value_boolean(a >  b);
        case TOK_GREATER_EQUAL: return make_value_boolean(a >= b);
        case TOK_DOUBLE_EQUAL: return make_value_boolean(a == b);
        case TOK_BANG_EQUAL: return make_value_boolean(a != b);
        default: return NULL_VALUE; // TODO: runtime error
        }
      } else {
        f64 a = (l.type == TYPE_INT) ? (f64)l.as.integer : l.as.floating_point;
        f64 b = (r.type == TYPE_INT) ? (f64)r.as.integer : r.as.floating_point;
        switch (expr->as.binary.op) {
        case TOK_PLUS: return make_value_float(a + b);
        case TOK_MINUS: return make_value_float(a - b);
        case TOK_STAR: return make_value_float(a * b);
        case TOK_SLASH: return make_value_float(a / b);
        case TOK_LESS: return make_value_boolean(a <  b);
        case TOK_LESS_EQUAL: return make_value_boolean(a <= b);
        case TOK_GREATER: return make_value_boolean(a >  b);
        case TOK_GREATER_EQUAL: return make_value_boolean(a >= b);
        case TOK_DOUBLE_EQUAL: return make_value_boolean(a == b);
        case TOK_BANG_EQUAL: return make_value_boolean(a != b);
        default: return NULL_VALUE;
        }
      }
    }

    assert(l.type == r.type); // TODO: Will cause problems later

    switch (l.type) {
    case TYPE_BOOLEAN:
      switch (expr->as.binary.op) {
      case TOK_LESS: return make_value_boolean(l.as.boolean < r.as.boolean);
      case TOK_LESS_EQUAL: return make_value_boolean(l.as.boolean <= r.as.boolean);
      case TOK_DOUBLE_EQUAL: return make_value_boolean(l.as.boolean == r.as.boolean);
      case TOK_BANG_EQUAL: return make_value_boolean(l.as.boolean != r.as.boolean);
      case TOK_GREATER: return make_value_boolean(l.as.boolean > r.as.boolean);
      case TOK_GREATER_EQUAL: return make_value_boolean(l.as.boolean >= r.as.boolean);
      case TOK_OR: return make_value_boolean(is_truthy(l) || is_truthy(r));
      case TOK_AND: return make_value_boolean(is_truthy(l) && is_truthy(r));
      default: return make_value_boolean(false);
      }
      break;
    case TYPE_STRING:
      switch (expr->as.binary.op) {
      case TOK_DOUBLE_EQUAL: return make_value_boolean(sv_is_equal(l.as.string, r.as.string));
      case TOK_BANG_EQUAL: return make_value_boolean(!sv_is_equal(l.as.string, r.as.string));
      case TOK_OR: return make_value_boolean(is_truthy(l) || is_truthy(r));
      case TOK_AND: return make_value_boolean(is_truthy(l) && is_truthy(r));
      default: return NULL_VALUE;
      }
      break;
    case TYPE_ARRAY: // TODO: Implement
    case TYPE_NULL: // TODO: Implement
    default:
      break;
    }
    break;
  }
  case EXPR_CALL: {
    if (expr->as.call.callee->kind != EXPR_IDENTIFIER) return NULL_VALUE; // TODO: Runtime error

    StringView identifier = expr->as.call.callee->as.identifier;
    NativeFn fn = map_identifier_to_native_fn(identifier);
    u32 count = expr->as.call.args->size;
    if (count > MAX_ARGS) {
      // TODO: Runtime error
      return NULL_VALUE;
    }
    Value values[MAX_ARGS];
    for (u32 i = 0; i < count; i++) {
      values[i] = eval_expr(context, expr->as.call.args->items[i]);
    }

    if (fn != NULL) {
      return fn(values, count);
    } else {
      Stmt *fn = find_fn(context->statements, identifier);
      if (fn == NULL) {
        return NULL_VALUE; // TODO: Runtime error
      }
      if (fn->as.fn_declare.param_count != count) {
        return NULL_VALUE; // TODO: Runtime error
      }

      Environment *fn_env = make_env(context);
      Environment *prev_env = context->cur_env;
      context->cur_env = fn_env;
      context->return_value = NULL_VALUE;

      for (u32 i = 0; i < fn->as.fn_declare.param_count; i++) {
        set_env_binding(fn_env, fn->as.fn_declare.param_identifiers[i], values[i]);
      }

      stmt_exec(context, *fn->as.fn_declare.body);

      context->returning = false;

      free_cur_env(context);
      context->cur_env = prev_env;

      return context->return_value;
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


static bool is_truthy(Value value) {
  switch (value.type) {
  case TYPE_INT:
    return value.as.integer > 0;
  case TYPE_FLOAT:
    return value.as.floating_point > 0.0;
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
        if (context->returning) {
          break;
        }
        stmt_exec(context, *stmt.as.for_loop.body);
      }
    } else {
      while (is_truthy(eval_expr(context, stmt.as.for_loop.condition))) {
        if (context->returning) {
          break;
        }
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
  case STMT_FN_DECLARE: {
    break;
  }
  case STMT_RETURN: {
    context->return_value = eval_expr(context, stmt.as.return_expr);
    context->returning = true;
    break;
  }
  }
}

static void stmt_array_exec(InterpreterContext *context, StmtArray *array) {
  for (u32 i = 0; i < array->size; i++) {
    if (context->returning) {
      break;
    }
    stmt_exec(context, array->items[i]);
  }
}

void interpreter_begin(InterpreterResult *result, StmtArray *statements) {
  InterpreterContext context = {0};
  context.statements = statements;
  context.arena = arena_init(1024 * 1000);
  context.errors = malloc(sizeof(LogArray));
  context.cur_env = make_env(&context);

  da_init(context.errors, 8);

  result->exit_code = 0;
  stmt_array_exec(&context, statements);

  da_free(context.errors);
  free_cur_env(&context); // TODO: Assumes we'll always return to the global env after finishing execution
  arena_free(context.arena);
  free(context.errors);
}

