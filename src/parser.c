#include "parser.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

Expr *expr_parse(ParseContext *context);
Expr *parse_add_sub(ParseContext *context);
Expr *parse_mul_div(ParseContext *context);
Expr *parse_unary(ParseContext *context);
Expr *parse_atom(ParseContext *context);

static Token peek(ParseContext *context) {
  return context->tokens->items[context->current];
}

Token advance(ParseContext *context) {
  return context->tokens->items[context->current++];
}

Expr *make_number(f64 value) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_NUMBER;
  e->as.number.value = value;
  return e;
}

Expr *make_unary(TokenKind op, Expr *right) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_UNARY;
  e->as.unary.op = op;
  e->as.unary.right = right;
  return e;
}

Expr *make_binary(Expr *left, TokenKind op, Expr *right) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_BINARY;
  e->as.binary.left = left;
  e->as.binary.op = op;
  e->as.binary.right = right;
  return e;
}

Expr *make_group(Expr *inner) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_GROUP;
  e->as.group.inner = inner;
  return e;
}

Expr *expr_parse(ParseContext *context) {
  return parse_add_sub(context);
}

Expr *parse_add_sub(ParseContext *context) {
  Expr *left = parse_mul_div(context);
  while (peek(context).kind == TOK_PLUS || peek(context).kind == TOK_MINUS) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_mul_div(context);
    left = make_binary(left, op, right);
  }
  return left;
}

Expr *parse_mul_div(ParseContext *context) {
  Expr *left = parse_unary(context);
  while (peek(context).kind == TOK_STAR || peek(context).kind == TOK_SLASH) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_unary(context);
    left = make_binary(left, op, right);
  }
  return left;
}

Expr *parse_unary(ParseContext *context) {
  while (peek(context).kind == TOK_MINUS) {
    TokenKind op = peek(context).kind;
    advance(context);
    Expr *right = parse_unary(context);
    return make_unary(op, right);
  }
  return parse_atom(context);
}

Expr *parse_atom(ParseContext *context) {
  // TODO: Allocate to arena, otherwise this will leak
  if (peek(context).kind == TOK_NUMBER) {
    u32 length = peek(context).lexeme.length;
    char *c = malloc(sizeof(char) * length);
    strncpy(c, peek(context).lexeme.start, length);
    advance(context);
    return make_number(atof(c));
  }
  if (peek(context).kind == TOK_LEFT_PAREN) {
    advance(context);
    Expr *inner = expr_parse(context);
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
    default:      return "?";
  }
}

void expr_print(Expr *e) {
  switch (e->kind) {
  case EXPR_NUMBER:
    printf("%g", e->as.number.value);
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

f64 expr_eval(Expr *expr) {
  switch (expr->kind) {
  case EXPR_NUMBER:
    return expr->as.number.value;
  case EXPR_UNARY:
    return -expr_eval(expr->as.unary.right);
  case EXPR_BINARY: {
    f64 l = expr_eval(expr->as.binary.left);
    f64 r = expr_eval(expr->as.binary.right);
    switch (expr->as.binary.op) {
    case TOK_PLUS: return l + r;
    case TOK_MINUS: return l - r;
    case TOK_STAR: return l * r;
    case TOK_SLASH: return l / r;
    default: return 0.0;
    }
  }
  default:
    return 0.0;
  }
}
