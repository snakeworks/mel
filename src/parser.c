#include "parser.h"
#include "base.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

Expr *parse_expr_start(ParserContext *context);
Expr *parse_comparison(ParserContext *context);
Expr *parse_add_sub(ParserContext *context);
Expr *parse_mul_div(ParserContext *context);
Expr *parse_unary(ParserContext *context);
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
  e->as.value.kind = VAL_NUMBER;
  e->as.value.as.number = value;
  return e;
}

Expr *make_expr_boolean(ParserContext *context, bool value) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_VALUE;
  e->as.value.kind = VAL_BOOLEAN;
  e->as.value.as.boolean = value;
  return e;
}

Expr *make_expr_string(ParserContext *context, StringView value) {
  Expr *e = arena_push(context->arena, Expr);
  e->kind = EXPR_VALUE;
  e->as.value.kind = VAL_STRING;
  e->as.value.as.string = value;
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
    Expr *condition = expr_parse(context);
    if (condition == NULL) {
      log_err(context->errors, cur_line(context), "Expected expression");
      return (Stmt){.kind = STMT_EMPTY};
    }
    Stmt *stmt = arena_push(context->arena, Stmt);
    *stmt = parse_statement(context);
    return (Stmt) {
      .kind = STMT_IF,
      .as.if_branch = {
        .condition = condition,
        .body = stmt
      }
    };
  }
  case TOK_IDENTIFIER: {
    advance(context);
  }
  default: {
    Expr *expr = expr_parse(context);
    expect(context, TOK_SEMICOLON, "Expected ';' after expression");
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
    case STMT_EXPR: expr_print(stmt->as.expr); printf("\n"); break;
    case STMT_BLOCK: printf("\n"); print_stmt_array(stmt->as.block, indent + 2); break;
    case STMT_IF: {
      printf("if ");
      expr_print(stmt->as.if_branch.condition);
      printf(" \n");
      print_stmt(stmt->as.if_branch.body, indent + 2);
      break;
    }
  }
}

void print_stmt_array(StmtArray *array, u8 indent) {
  for (u32 i = 0; i < array->size; i++) {
    print_stmt(&array->items[i], indent);
  }
}

Expr *expr_parse(ParserContext *context) {
  Expr *e = parse_expr_start(context);
  return e;
}

Expr *parse_expr_start(ParserContext *context) {
  return parse_comparison(context);
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
  return parse_atom(context);
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
  }

  if (peek(context).kind == TOK_LEFT_PAREN) {
    advance(context);
    Expr *inner = parse_expr_start(context);
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
    default: return "?";
  }
}


void expr_print(Expr *e) {
  switch (e->kind) {
  case EXPR_VALUE:
    value_print(e->as.value);
    break;
  case EXPR_BINARY:
    printf("(%s ", op_string(e->as.binary.op));
    expr_print(e->as.binary.left);
    printf(" ");
    expr_print(e->as.binary.right);
    printf(")");
    break;
  case EXPR_UNARY:
    printf("(%s ", op_string(e->as.unary.op));
    expr_print(e->as.unary.right);
    printf(")");
    break;
  default:
    break;
  }
}

