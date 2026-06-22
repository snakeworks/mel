#include "parser.h"
#include "base.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

Expr *parse_expr(ParserContext *context);
Expr *parse_comparison(ParserContext *context);
Expr *parse_or(ParserContext *context);
Expr *parse_and(ParserContext *context);
Expr *parse_add_sub(ParserContext *context);
Expr *parse_mul_div(ParserContext *context);
Expr *parse_unary(ParserContext *context);
Expr *parse_call(ParserContext *context);
Expr *parse_atom(ParserContext *context);
Stmt parse_statement(ParserContext *context);
void parse_program(ParserContext *context);

static Token peek(ParserContext *context) {
  return context->tokens->items[context->current];
}

static Token peek_next(ParserContext *context) {
  return context->tokens->items[context->current + 1];
}

static u32 cur_line(ParserContext *context) {
  return context->tokens->items[context->current].line + 1;
}

static Token advance(ParserContext *context) {
  return context->tokens->items[context->current++];
}

static bool seek(ParserContext *context, TokenKind token) {
  while (peek(context).kind != token) {
    if (peek(context).kind == TOK_EOF) {
      return false;
    }
    advance(context);
  }
  return true;
}

static Token expect(ParserContext *context, TokenKind token, const char *msg, ...) {
  if (peek(context).kind == token) return advance(context);
  va_list list;
  va_start(list, msg);
  char *ptr = arena_alloc_vformat(context->arena, msg, list);
  va_end(list);
  log_err(context->errors, cur_line(context), ptr);
  if (seek(context, TOK_SEMICOLON)) advance(context);
  return peek(context);
}

Expr *make_expr_number(ParserContext *context, f64 value) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_VALUE;
  e->as.value.type = TYPE_NUMBER;
  e->as.value.as.number = value;
  return e;
}

Expr *make_expr_boolean(ParserContext *context, bool value) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_VALUE;
  e->as.value.type = TYPE_BOOLEAN;
  e->as.value.as.boolean = value;
  return e;
}

Expr *make_expr_string(ParserContext *context, StringView value) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_VALUE;
  e->as.value.type = TYPE_STRING;
  e->as.value.as.string = value;
  return e;
}

Expr *make_expr_array(ParserContext *context, ExprArray *arr) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_VALUE;
  e->as.value.type = TYPE_ARRAY;
  e->as.value.as.array = arr;
  return e;
}

Expr *make_unary(ParserContext *context, TokenKind op, Expr *right) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_UNARY;
  e->as.unary.op = op;
  e->as.unary.right = right;
  return e;
}

Expr *make_binary(ParserContext *context, Expr *left, TokenKind op, Expr *right) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_BINARY;
  e->as.binary.left = left;
  e->as.binary.op = op;
  e->as.binary.right = right;
  return e;
}

Expr *make_identifier(ParserContext *context, StringView name) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_IDENTIFIER;
  e->as.identifier = name;
  return e;
}

Expr *make_call(ParserContext *context, Expr *callee, ExprArray *args) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_CALL;
  e->as.call.callee = callee;
  e->as.call.args = args;
  return e;
}

Expr *make_iter(ParserContext *context, StringView el_identifier, Expr *iterable) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_ITER;
  e->as.iter.el_identifier = el_identifier;
  e->as.iter.iterable = iterable;
  e->as.iter.cur_index = 0;
  return e;
}

void parser_begin(ParserResult *result, TokenArray *tokens, Arena *arena) {
  StmtArray *statements = malloc(sizeof(StmtArray));
  result->statements = statements;
  da_init(statements, 8);

  result->errors = malloc(sizeof(LogArray));
  da_init(result->errors, 8);
  ParserContext context = {
    .tokens = tokens,
    .statements = statements,
    .errors = result->errors,
    .arena = arena,
    .current = 0
  };
  parse_program(&context);
}

void parse_program(ParserContext *context) {
  while (peek(context).kind != TOK_EOF) {
    Stmt stmt = parse_statement(context);
    da_append(context->statements, stmt);
  }
}

Stmt parse_statement(ParserContext *context) {
  switch (peek(context).kind) {
  case TOK_LEFT_BRACE: {
    advance(context);
    StmtArray *block = malloc(sizeof(StmtArray));
    da_init(block, 8);
    while (peek(context).kind != TOK_RIGHT_BRACE && peek(context).kind != TOK_EOF) {
      da_append(block, parse_statement(context));
    }
    if (peek(context).kind == TOK_RIGHT_BRACE) {
      advance(context);
    } else {
      log_err(context->errors, cur_line(context), "Expected closing brace");
    }
    StmtArray *block_arena_copy;
    da_copy_to_arena(block_arena_copy, block, context->arena, StmtArray);
    da_free(block);
    free(block);
    return (Stmt){
      .kind = STMT_BLOCK,
      .as.block = block_arena_copy
    };
  }
  case TOK_RIGHT_BRACE: {
    log_err(context->errors, cur_line(context), "Unexpected closing brace with no opening brace");
    advance(context);
    return (Stmt){.kind = STMT_EMPTY};
  }
  case TOK_IF: {
    advance(context);
    Expr *condition = parse_expr(context);
    if (condition == NULL) {
      log_err(context->errors, cur_line(context), "Expected expression");
      return (Stmt){.kind = STMT_EMPTY};
    }
    Stmt *body = arena_push(context->arena, Stmt);
    *body = parse_statement(context);
    Stmt *else_body = arena_push(context->arena, Stmt);
    if (peek(context).kind == TOK_ELSE) {
      advance(context);
      *else_body = parse_statement(context);
    }
    return (Stmt) {
      .kind = STMT_IF,
      .as.if_branch = {
        .condition = condition,
        .body = body,
        .else_body = else_body
      }
    };
  }
  case TOK_FOR: {
    advance(context);
    Expr *condition = parse_expr(context);
    Stmt *stmt = arena_push(context->arena, Stmt);
    *stmt = parse_statement(context);
    return (Stmt){
      .kind = STMT_FOR,
      .as.for_loop = {
        .condition = condition,
        .body = stmt,
      }
    };
  }
  case TOK_FN: {
    advance(context);
    StringView identifier = expect(context, TOK_IDENTIFIER, "Expected identifier after function keyword").lexeme;
    expect(context, TOK_LEFT_PAREN, "Expected opening parenthesis after function name");

    u32 param_count = 0;
    u32 loc_tok_index = context->current;
    while (context->tokens->items[loc_tok_index].kind != TOK_RIGHT_PAREN) {
      if (context->tokens->items[loc_tok_index].kind == TOK_IDENTIFIER) {
        param_count++;
      }
      loc_tok_index++;
    }

    StringView *arg_identifiers = arena_alloc(context->arena, sizeof(StringView)*param_count, alignof(StringView));

    u32 param_index = 0;
    while (peek(context).kind != TOK_RIGHT_PAREN) {
      Token tok = advance(context);
      if (tok.kind != TOK_IDENTIFIER) {
        continue;
      }
      arg_identifiers[param_index++] = tok.lexeme;
    }

    expect(context, TOK_RIGHT_PAREN, "Expected closing parenthesis after function arguments");

    Stmt *body = arena_push(context->arena, Stmt);
    *body = parse_statement(context);

    return (Stmt) {
      .kind = STMT_FN_DECLARE,
      .as.fn_declare = {
        .identifier = identifier,
        .param_identifiers = arg_identifiers,
        .param_count = param_count,
        .body = body
      }
    };
  }
  case TOK_RETURN: {
    advance(context);
    Expr *expr = NULL;
    if (peek(context).kind != TOK_SEMICOLON) {
      expr = parse_expr(context);
    }
    expect(context, TOK_SEMICOLON, "Expected ';' after return statement");
    return (Stmt) {
      .kind = STMT_RETURN,
      .as.return_expr = expr
    };
  }
  case TOK_IDENTIFIER: {
    if (peek_next(context).kind == TOK_EQUAL) {
      Expr *identifier = parse_expr(context);
      advance(context);
      Expr *assignment = parse_expr(context);
      expect(context, TOK_SEMICOLON, "Expected ';' after expression");
      return (Stmt){
        .kind = STMT_ASSIGN,
        .as.assign = {
          .identifier = identifier->as.identifier,
          .assignment = assignment
        }
      };
    }
    [[fallthrough]]; // Let case fall through
  }
  default: {
    Expr *expr = parse_expr(context);
    if (expr->kind != EXPR_IDENTIFIER) {
      expect(context, TOK_SEMICOLON, "Expected ';' after expression");
    } else {
      advance(context);
    }
    return (Stmt){
      .kind = STMT_EXPR,
      .as.expr = expr
    };
  }
  }
  return (Stmt){.kind = STMT_EMPTY};
}

static const char *stmt_kind_to_str(StmtKind kind) {
  switch (kind) {
  CASE_STRING(STMT_EMPTY);
  CASE_STRING(STMT_EXPR);
  CASE_STRING(STMT_BLOCK);
  CASE_STRING(STMT_IF);
  CASE_STRING(STMT_FOR);
  CASE_STRING(STMT_ASSIGN);
  CASE_STRING(STMT_FN_DECLARE);
  CASE_STRING(STMT_RETURN);
  }
  return "";
}

static void print_stmt(Stmt *stmt, u8 indent) {
  printf(
    "%*s%s ",
    indent, "",
    stmt_kind_to_str(stmt->kind)
  );
  switch (stmt->kind) {
    case STMT_EMPTY: printf("(empty)\n"); break;
    case STMT_EXPR: print_expr(stmt->as.expr); printf("\n"); break;
    case STMT_BLOCK: printf("\n"); print_stmt_array(stmt->as.block, indent + 2); break;
    case STMT_IF: {
      printf("if ");
      print_expr(stmt->as.if_branch.condition);
      printf(" \n");
      print_stmt(stmt->as.if_branch.body, indent + 2);
      print_stmt(stmt->as.if_branch.else_body, indent + 2);
      break;
    }
    case STMT_FOR: {
      printf("for ");
      print_expr(stmt->as.for_loop.condition);
      printf(" \n");
      print_stmt(stmt->as.for_loop.body, indent + 2);
      break;
    }
    case STMT_ASSIGN: {
      printf(SV_FMT" = ", SV_ARG(stmt->as.assign.identifier));
      print_expr(stmt->as.assign.assignment);
      printf("\n");
      break;
    }
    case STMT_FN_DECLARE: {
      printf(SV_FMT"(", SV_ARG(stmt->as.fn_declare.identifier));
      for (u32 i = 0; i < stmt->as.fn_declare.param_count; i++) {
        printf(SV_FMT, SV_ARG(stmt->as.fn_declare.param_identifiers[i]));
        if (i < stmt->as.fn_declare.param_count-1) {
          printf(", ");
        }
      }
      printf(")\n");
      print_stmt(stmt->as.fn_declare.body, indent + 2);
      break;
    }
    case STMT_RETURN: {
      printf("return ");
      print_expr(stmt->as.return_expr);
      printf("\n");
      break;
    }
  }
}

void print_stmt_array(StmtArray *array, u8 indent) {
  for (u32 i = 0; i < array->size; i++) {
    print_stmt(&array->items[i], indent);
  }
}

Expr *parse_expr(ParserContext *context) {
  return parse_or(context);
}

Expr *parse_or(ParserContext *context) {
  Expr *left = parse_and(context);
  while (peek(context).kind == TOK_OR) {
    advance(context);
    Expr *right = parse_and(context);
    left = make_binary(context, left, TOK_OR, right);
  }
  return left;
}

Expr *parse_and(ParserContext *context) {
  Expr *left = parse_comparison(context);
  while (peek(context).kind == TOK_AND) {
    advance(context);
    Expr *right = parse_comparison(context);
    left = make_binary(context, left, TOK_AND, right);
  }
  return left;
}

Expr *parse_comparison(ParserContext *context) {
  Expr *left = parse_add_sub(context);
  while (
    peek(context).kind == TOK_LESS ||
    peek(context).kind == TOK_LESS_EQUAL ||
    peek(context).kind == TOK_DOUBLE_EQUAL ||
    peek(context).kind == TOK_BANG_EQUAL ||
    peek(context).kind == TOK_GREATER ||
    peek(context).kind == TOK_GREATER_EQUAL
  ) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_add_sub(context);
    left = make_binary(context, left, op, right);
  }
  return left;
}

Expr *parse_add_sub(ParserContext *context) {
  Expr *left = parse_mul_div(context);
  while (peek(context).kind == TOK_PLUS || peek(context).kind == TOK_MINUS) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_mul_div(context);
    left = make_binary(context, left, op, right);
  }
  return left;
}

Expr *parse_mul_div(ParserContext *context) {
  Expr *left = parse_unary(context);
  while (peek(context).kind == TOK_STAR || peek(context).kind == TOK_SLASH) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_unary(context);
    left = make_binary(context, left, op, right);
  }
  return left;
}

Expr *parse_unary(ParserContext *context) {
  while (peek(context).kind == TOK_MINUS) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_unary(context);
    return make_unary(context, op, right);
  }
  return parse_call(context);
}

Expr *parse_call(ParserContext *context) {
  Expr *expr = parse_atom(context);
  if (expr == NULL) {
    return NULL;
  }

  while (peek(context).kind == TOK_LEFT_PAREN) {
    advance(context);

    ExprArray *args = malloc(sizeof(ExprArray));
    da_init(args, 4);

    if (peek(context).kind != TOK_RIGHT_PAREN) {
      do {
        da_append(args, parse_expr(context));
      } while (peek(context).kind == TOK_COMMA && (advance(context), true));
    }

    expect(context, TOK_RIGHT_PAREN, "Expected ')' after arguments");

    ExprArray *args_copy;
    da_copy_to_arena(args_copy, args, context->arena, ExprArray);
    da_free(args);
    free(args);

    expr = make_call(context, expr, args_copy);
  }
  return expr;
}

Expr *parse_atom(ParserContext *context) {
  if (peek(context).kind == TOK_NUMBER) {
    return make_expr_number(context, sv_to_f64(advance(context).lexeme));
  } else if (
    peek(context).kind == TOK_TRUE ||
    peek(context).kind == TOK_FALSE
  ) {
    bool value = peek(context).kind == TOK_TRUE;
    advance(context);
    return make_expr_boolean(context, value);
  } else if (peek(context).kind == TOK_STRING) {
    return make_expr_string(context, advance(context).lexeme);
  } else if (peek(context).kind == TOK_IDENTIFIER) {
    if (peek_next(context).kind == TOK_IN) {
      StringView el_identifier = advance(context).lexeme;
      advance(context);
      Expr *iterable = parse_expr(context);
      return make_iter(context, el_identifier, iterable);
    }
    return make_identifier(context, advance(context).lexeme);
  } else if (peek(context).kind == TOK_LEFT_BRACKET) {
    advance(context);
    // The struct lives in the arena, but its items buffer stays on the malloc
    // heap so it can be grown with realloc by array_append at runtime. Copying
    // it into the arena (da_copy_to_arena) would make da_append realloc an
    // arena pointer, which is undefined behavior and segfaults
    ExprArray *arr = arena_push(context->arena, ExprArray);
    da_init(arr, 4);
    while (peek(context).kind != TOK_RIGHT_BRACKET) {
      Expr *el = parse_expr(context);
      da_append(arr, el);
      if (peek(context).kind != TOK_RIGHT_BRACKET) {
        expect(context, TOK_COMMA, "Expected ',' after array element");
      }
    }
    advance(context);
    return make_expr_array(context, arr);
  }

  if (peek(context).kind == TOK_LEFT_PAREN) {
    advance(context);
    Expr *inner = parse_expr(context);
    expect(context, TOK_RIGHT_PAREN, "Expected closing parenthesis");
    return inner;
  }
  return NULL;
}

static const char* op_string(TokenKind t) {
  switch (t) {
    case TOK_PLUS:  return "+";
    case TOK_MINUS: return "-";
    case TOK_STAR:  return "*";
    case TOK_SLASH: return "/";
    case TOK_LESS: return "<";
    case TOK_LESS_EQUAL: return "<=";
    case TOK_DOUBLE_EQUAL: return "==";
    case TOK_BANG_EQUAL: return "!=";
    case TOK_GREATER: return ">";
    case TOK_GREATER_EQUAL: return ">=";
    case TOK_OR: return "or";
    case TOK_AND: return "and";
    default: return "?";
  }
}

void print_value(Value value) {
  switch (value.type) {
  case TYPE_NUMBER:
    printf("%g", value.as.number);
    break;
  case TYPE_BOOLEAN:
    printf(value.as.boolean ? "true" : "false");
    break;
  case TYPE_STRING:
    printf(SV_FMT, SV_ARG(value.as.string));
    break;
  case TYPE_ARRAY:
    printf("[");
    for (u32 i = 0; i < value.as.array->size; i++) {
      print_expr(value.as.array->items[i]);
      if (i < value.as.array->size-1) {
        printf(", ");
      }
    }
    printf("]");
    break;
  case TYPE_NULL:
    printf("null");
    break;
  }
}

void print_expr(Expr *e) {
  if (e == NULL) {
    return;
  }
  switch (e->kind) {
  case EXPR_VALUE:
    print_value(e->as.value);
    break;
  case EXPR_IDENTIFIER:
    printf(SV_FMT, SV_ARG(e->as.identifier));
    break;
  case EXPR_BINARY:
    printf("(%s ", op_string(e->as.binary.op));
    print_expr(e->as.binary.left);
    printf(" ");
    print_expr(e->as.binary.right);
    printf(")");
    break;
  case EXPR_UNARY:
    printf("(%s ", op_string(e->as.unary.op));
    print_expr(e->as.unary.right);
    printf(")");
    break;
  case EXPR_CALL:
    print_expr(e->as.call.callee);
    printf("(");
    for (u32 i = 0; i < e->as.call.args->size; i++) {
      print_expr(e->as.call.args->items[i]);
      if (i < e->as.call.args->size-1) {
        printf(", ");
      }
    }
    printf(")");
    break;
  case EXPR_ITER:
    printf(SV_FMT" in ", SV_ARG(e->as.iter.el_identifier));
    print_expr(e->as.iter.iterable);
    break;
  default:
    break;
  }
}

