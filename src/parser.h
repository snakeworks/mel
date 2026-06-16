#ifndef PARSER_H_
#define PARSER_H_

#include "base.h"
#include "lexer.h"

typedef enum {
  VAL_NUMBER,
  VAL_BOOLEAN,
  VAL_STRING,
  VAL_NULL,
} ValueKind;

typedef struct {
  ValueKind kind;
  union {
    f64 number;
    bool boolean;
    StringView string;
  } as;
} Value;

static const Value NULL_VALUE = {.kind = VAL_NULL};

typedef enum {
  EXPR_VALUE,
  EXPR_UNARY,
  EXPR_BINARY,
} ExprKind;

typedef struct Expr Expr;

struct Expr {
  ExprKind kind;
  union {
    Value value;
    struct { TokenKind op; Expr *right; } unary;
    struct { Expr *left; TokenKind op; Expr *right; } binary;
    struct { Expr *inner; } group;
  } as;
};

typedef struct {
  TokenArray *tokens;
  u32 current;
} ParseContext;

Expr *expr_parse(ParseContext *context);
void expr_print(Expr *expr);
Value expr_eval(Expr *expr);
void value_print(Value value);

#endif
