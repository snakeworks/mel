#ifndef PARSER_H_
#define PARSER_H_

#include "base.h"
#include "lexer.h"

typedef enum {
  EXPR_NUMBER,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_GROUP,
} ExprKind;

typedef struct Expr Expr;

struct Expr {
  ExprKind kind;
  union {
    struct { f64 value; } number;
    struct { TokenKind op; Expr *right; } unary;
    struct { Expr *left; TokenKind op; Expr *right; } binary;
    struct { Expr *inner; } group;
  } as;
};

typedef struct {
  TokenArray *tokens;
  u32 current;
} ParseContext;

Expr *parser_start(ParseContext *context);
void expr_print(Expr *expr);
f64 expr_eval(Expr *expr);

#endif
