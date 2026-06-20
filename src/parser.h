#ifndef PARSER_H_
#define PARSER_H_

#include "base.h"
#include "lexer.h"

typedef enum {
  TYPE_NUMBER,
  TYPE_BOOLEAN,
  TYPE_STRING,
  TYPE_NULL,
} Type;

typedef struct {
  Type type;
  union {
    f64 number;
    bool boolean;
    StringView string;
  } as;
} Value;

static const Value NULL_VALUE = {.type = TYPE_NULL};

typedef enum {
  EXPR_VALUE,
  EXPR_IDENTIFIER,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_CALL
} ExprKind;

typedef struct Expr Expr;

typedef struct {
  Expr **items;
  u32 capacity;
  u32 size;
} ExprArray;

struct Expr {
  ExprKind kind;
  union {
    Value value;
    StringView identifier;
    struct { TokenKind op; Expr *right; } unary;
    struct { Expr *left; TokenKind op; Expr *right; } binary;
    struct { Expr *callee; ExprArray *args; } call;
  } as;
};

typedef enum {
  STMT_EMPTY,
  STMT_EXPR,
  STMT_BLOCK,
  STMT_IF,
  STMT_FOR,
  STMT_ASSIGN,
} StmtKind;

typedef struct Stmt Stmt;
typedef struct StmtArray StmtArray;

struct Stmt {
  StmtKind kind;
  union {
    Expr *expr;
    StmtArray *block;
    struct { Expr *condition; Stmt *body; } if_branch;
    struct { Expr *condition; Stmt *body; } for_loop;
    struct { StringView identifier; Expr *assignment; } assign;
  } as;
};

struct StmtArray {
  Stmt *items;
  u32 capacity;
  u32 size;
};

typedef struct {
  TokenArray *tokens;
  StmtArray *statements;
  LogArray *errors;
  Arena *arena;
  u32 current;
} ParserContext;

typedef struct {
  StmtArray *statements;
  LogArray *errors;
} ParserResult;

void parser_begin(ParserResult *result, TokenArray *tokens, Arena *arena);
void print_stmt_array(StmtArray *array, u8 indent);
void print_expr(Expr *expr);
void print_value(Value value);

#endif
