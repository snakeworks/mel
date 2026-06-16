#include "lexer.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

const char *token_to_str(Token token) {
  switch (token.kind) {
    CASE_STRING(TOK_LEFT_PAREN);
    CASE_STRING(TOK_RIGHT_PAREN);
    CASE_STRING(TOK_LEFT_BRACE);
    CASE_STRING(TOK_RIGHT_BRACE);
    CASE_STRING(TOK_COMMA);
    CASE_STRING(TOK_DOT);
    CASE_STRING(TOK_MINUS);
    CASE_STRING(TOK_PLUS);
    CASE_STRING(TOK_SLASH);
    CASE_STRING(TOK_STAR);
    CASE_STRING(TOK_BANG);
    CASE_STRING(TOK_BANG_EQUAL);
    CASE_STRING(TOK_EQUAL);
    CASE_STRING(TOK_DOUBLE_EQUAL);
    CASE_STRING(TOK_GREATER);
    CASE_STRING(TOK_GREATER_EQUAL);
    CASE_STRING(TOK_LESS);
    CASE_STRING(TOK_LESS_EQUAL);
    CASE_STRING(TOK_IDENTIFIER);
    CASE_STRING(TOK_STRING);
    CASE_STRING(TOK_NUMBER);
    CASE_STRING(TOK_AND);
    CASE_STRING(TOK_OR);
    CASE_STRING(TOK_IF);
    CASE_STRING(TOK_ELSE);
    CASE_STRING(TOK_TRUE);
    CASE_STRING(TOK_FALSE);
    CASE_STRING(TOK_FUNC);
    CASE_STRING(TOK_FOR);
    CASE_STRING(TOK_NULL);
    CASE_STRING(TOK_RETURN);
    CASE_STRING(TOK_EOF);
  }
  return "Unknown token";
}

static char peek(const char *source, u32 current) {
  current++;
  return source[current];
}

void add_token(TokenArray *array, TokenKind kind, const char *lexeme_start, u32 lexeme_length, u32 line) {
  StringView s = {.start = lexeme_start, .length = lexeme_length};
  Token t = {.kind = kind, .lexeme = s, .line = line};
  da_append(array, t);
}

void seek(const char *source, u32 *current, char symbol) {
  while (source[*current] != symbol && source[*current] != '\0') {
    (*current)++;
  }
}

TokenKind str_to_identifier_kind(const char *start, u32 length) {
  if (strncmp(start, "and", length) == 0) return TOK_AND;
  else if (strncmp(start, "or", length) == 0) return TOK_OR;
  else if (strncmp(start, "if", length) == 0) return TOK_IF;
  else if (strncmp(start, "else", length) == 0) return TOK_ELSE;
  else if (strncmp(start, "true", length) == 0) return TOK_TRUE;
  else if (strncmp(start, "false", length) == 0) return TOK_FALSE;
  else if (strncmp(start, "func", length) == 0) return TOK_FUNC;
  else if (strncmp(start, "for", length) == 0) return TOK_FOR;
  else if (strncmp(start, "null", length) == 0) return TOK_NULL;
  else if (strncmp(start, "return", length) == 0) return TOK_RETURN;
  return TOK_IDENTIFIER;
}

void tokenize(TokenArray *array, const char *source) {
  da_init(array, 32);

  u32 current = 0;
  u32 line = 0;

  while (source[current] != '\0') {
    switch (source[current]) {
    case '(': add_token(array, TOK_LEFT_PAREN, &source[current], 1, line); break;
    case ')': add_token(array, TOK_RIGHT_PAREN, &source[current], 1, line); break;
    case '{': add_token(array, TOK_LEFT_BRACE, &source[current], 1, line); break;
    case '}': add_token(array, TOK_RIGHT_BRACE, &source[current], 1, line); break;
    case ',': add_token(array, TOK_COMMA, &source[current], 1, line); break;
    case '.': add_token(array, TOK_DOT, &source[current], 1, line); break;
    case '+': add_token(array, TOK_PLUS, &source[current], 1, line); break;
    case '-': add_token(array, TOK_MINUS, &source[current], 1, line); break;
    case '*': add_token(array, TOK_STAR, &source[current], 1, line); break;
    case '/': {
      if (peek(source, current) == '/') {
        seek(source, &current, '\n');
        line++;
      } else {
        add_token(array, TOK_SLASH, &source[current], 1, line);
      }
      break;
    }
    case '=': {
      bool matches = peek(source, current) == '=';
      add_token(array, matches ? TOK_DOUBLE_EQUAL : TOK_EQUAL, &source[current], matches ? 2 : 1, line);
      if (matches) current++;
      break;
    }
    case '>': {
      bool matches = peek(source, current) == '=';
      add_token(array, matches ? TOK_GREATER_EQUAL : TOK_GREATER, &source[current], matches ? 2 : 1, line);
      if (matches) current++;
      break;
    }
    case '<': {
      bool matches = peek(source, current) == '=';
      add_token(array, matches ? TOK_LESS_EQUAL : TOK_LESS, &source[current], matches ? 2 : 1, line);
      if (matches) current++;
      break;
    }
    case '!': {
      bool matches = peek(source, current) == '=';
      add_token(array, matches ? TOK_BANG_EQUAL : TOK_BANG, &source[current], matches ? 2 : 1, line);
      if (matches) current++;
      break;
    }
    case '"': {
      current++;
      u32 start = current;
      seek(source, &current, '"');
      u32 length = current - start;
      add_token(array, TOK_STRING, &source[start], length, line);
    }
    case ' ':
    case '\r':
    case '\t':
      break;
    case '\n': line++; break;
    default: {
      if (isdigit(source[current])) {
        u32 start = current;

        while (isdigit(source[current])) current++;

        if (source[current] == '.' && isdigit(peek(source, current))) {
          current++;
          while (isdigit(source[current])) current++;
        }

        u32 length = current - start;
        current--; // So we don't increase current twice loop
        add_token(array, TOK_NUMBER, &source[start], length, line);

        break;
      } else if (isalpha(source[current])) {
        u32 start = current;
        while (isalpha(source[current])) current++;
        u32 length = current - start;
        current--;
        TokenKind kind = str_to_identifier_kind(&source[start], length);
        add_token(array, kind, &source[start], length, line);
      }
    }
    }

    current++;
  }

  add_token(array, TOK_EOF, &source[current], 1, line);
}

void print_token_array(TokenArray *array) {
  for (u32 i = 0; i < array->size; i++) {
    printf("%s " SV_FMT " (line %d)\n", token_to_str(array->items[i]), SV_ARG(array->items[i].lexeme), array->items[i].line);
  }
}
