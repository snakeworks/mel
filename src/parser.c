#include "parser.h"
#include "base.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Expr *expr_parse(ParseContext *context);
Expr *parse_comparison(ParseContext *context);
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

Expr *make_expr_number(f64 value) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_VALUE;
  e->as.value.kind = VAL_NUMBER;
  e->as.value.as.number = value;
  return e;
}

Expr *make_expr_boolean(bool value) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_VALUE;
  e->as.value.kind = VAL_BOOLEAN;
  e->as.value.as.boolean = value;
  return e;
}

Expr *make_expr_string(StringView value) {
  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_VALUE;
  e->as.value.kind = VAL_STRING;
  e->as.value.as.string = value;
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

Value make_value_number(f64 value) {
  Value v;
  v.kind = VAL_NUMBER;
  v.as.number = value;
  return v;
}

Value make_value_boolean(bool value) {
  Value v;
  v.kind = VAL_BOOLEAN;
  v.as.boolean = value;
  return v;
}

Value make_value_string(StringView value) {
  Value v;
  v.kind = VAL_STRING;
  v.as.string = value;
  return v;
}

Expr *expr_parse(ParseContext *context) {
  return parse_comparison(context);
}

Expr *parse_comparison(ParseContext *context) {
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
    left = make_binary(left, op, right);
  }
  return left;
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
    return make_expr_number(sv_to_f64(advance(context).lexeme));
  } else if (
    peek(context).kind == TOK_TRUE ||
    peek(context).kind == TOK_FALSE
  ) {
    bool value = peek(context).kind == TOK_TRUE;
    advance(context);
    return make_expr_boolean(value);
  } else if (peek(context).kind == TOK_STRING) {
    return make_expr_string(advance(context).lexeme);
  }

  if (peek(context).kind == TOK_LEFT_PAREN) {
    advance(context);
    Expr *inner = expr_parse(context);
    advance(context);
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

Value expr_eval(Expr *expr) {
  switch (expr->kind) {
  case EXPR_VALUE:
    return expr->as.value;
  case EXPR_UNARY: {
    Value v = expr_eval(expr->as.unary.right);
    if (expr->as.unary.op == TOK_MINUS) {
      return make_value_number(-v.as.number);
    }
    return v;
  }
  case EXPR_BINARY: {
    Value l = expr_eval(expr->as.binary.left);
    Value r = expr_eval(expr->as.binary.right);
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
