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

typedef enum {
  STMT_EXPR
} StmtKind;

typedef struct {
  TokenArray *tokens;
  LogArray *errors;
  Arena *arena;
  u32 current;
} ParserContext;

typedef struct {
  // TODO: Include list of statements and remove expr
  Expr *expr;
  LogArray *errors;
} ParserResult;

void parser_begin(ParserResult *result, TokenArray *tokens, Arena *arena);
Expr *expr_parse(ParserContext *context);
void expr_print(Expr *expr);
Value expr_eval(Expr *expr);
void value_print(Value value);

#endif
