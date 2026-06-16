#ifndef LEXER_H_
#define LEXER_H_

#include "base.h"

typedef enum {
  TOK_LEFT_PAREN,
  TOK_RIGHT_PAREN,
  TOK_LEFT_BRACE,
  TOK_RIGHT_BRACE,
  TOK_COMMA,
  TOK_DOT,
  TOK_MINUS,
  TOK_PLUS,
  TOK_SLASH,
  TOK_STAR,

  TOK_BANG,
  TOK_BANG_EQUAL,
  TOK_EQUAL,
  TOK_DOUBLE_EQUAL,
  TOK_GREATER,
  TOK_GREATER_EQUAL,
  TOK_LESS,
  TOK_LESS_EQUAL,

  TOK_IDENTIFIER,
  TOK_STRING,
  TOK_NUMBER,

  TOK_AND,
  TOK_OR,
  TOK_IF,
  TOK_ELSE,
  TOK_TRUE,
  TOK_FALSE,
  TOK_FUNC,
  TOK_FOR,
  TOK_NULL,
  TOK_RETURN,

  TOK_EOF
} TokenKind;

typedef struct {
  TokenKind kind;
  StringView lexeme;
  u32 line;
} Token;

typedef struct {
  Token *items;
  u32 size;
  u32 capacity;
} TokenArray;

void tokenize(TokenArray *array, const char *source);
void print_token_array(TokenArray *array);

#endif
