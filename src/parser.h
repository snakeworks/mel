#ifndef PARSER_H_
#define PARSER_H_

#include "base.h"
#include "lexer.h"

typedef enum {
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_BOOLEAN,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_NULL,
} Type;

typedef struct Expr Expr;
typedef struct ExprArray ExprArray;

typedef struct {
  Type type;
  union {
    i64 integer;
    f64 floating_point;
    bool boolean;
    StringView string;
    ExprArray *array;
  } as;
} Value;

static const Value NULL_VALUE = {.type = TYPE_NULL};

typedef enum {
  EXPR_VALUE,
  EXPR_IDENTIFIER,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_CALL,
  EXPR_ITER,
} ExprKind;

struct ExprArray {
  Expr **items;
  u32 capacity;
  u32 size;
};

struct Expr {
  ExprKind kind;
  union {
    Value value;
    StringView identifier;
    struct { TokenKind op; Expr *right; } unary;
    struct { Expr *left; TokenKind op; Expr *right; } binary;
    struct { Expr *callee; ExprArray *args; } call;
    struct { StringView el_identifier; Expr *iterable; u64 cur_index; } iter;
  } as;
};

typedef enum {
  STMT_EMPTY,
  STMT_EXPR,
  STMT_BLOCK,
  STMT_IF,
  STMT_FOR,
  STMT_ASSIGN,
  STMT_FN_DECLARE,
  STMT_RETURN
} StmtKind;

typedef struct Stmt Stmt;
typedef struct StmtArray StmtArray;

struct Stmt {
  StmtKind kind;
  union {
    Expr *expr;
    StmtArray *block;
    struct { Expr *condition; Stmt *body; Stmt *else_body; } if_branch;
    struct { Expr *condition; Stmt *body; } for_loop;
    struct { StringView identifier; Expr *assignment; } assign;
    struct { StringView identifier; StringView *param_identifiers; u64 param_count; Stmt *body; } fn_declare;
    Expr *return_expr;
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
