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
    CASE_STRING(TOK_COLON);
    CASE_STRING(TOK_SEMICOLON);
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

static const char *peek(LexerContext *context) {
  return &context->source[context->current];
}

static const char *peek_next(LexerContext *context) {
  return &context->source[context->current + 1];
}

static const char *advance(LexerContext *context) {
  return &context->source[context->current++];
}

void add_token(LexerContext *context, TokenKind kind, const char *lexeme_start, u32 lexeme_length) {
  StringView s = {.start = lexeme_start, .length = lexeme_length};
  Token t = {.kind = kind, .lexeme = s, .line = context->line};
  da_append(context->tokens, t);
}

bool seek(LexerContext *context, char symbol) {
  while (*peek(context) != symbol) {
    if (*peek(context) == '\0') {
      char *ptr = arena_alloc_format(context->arena, "Expected '%c', but found end of input", symbol);
      log_err(context->errors, context->line + 1, ptr);
      return false;
    } else if (*peek(context) == '\n') {
      context->line++;
    }
    context->current++;
  }
  return true;
}

static bool kw_is_equal(const char *start, const char *kw, u32 length) {
  return strlen(kw) == length && strncmp(start, kw, length) == 0;
}

TokenKind str_to_identifier_kind(const char *start, u32 length) {
  if (kw_is_equal(start, "and", length)) return TOK_AND;
  else if (kw_is_equal(start, "or", length)) return TOK_OR;
  else if (kw_is_equal(start, "if", length)) return TOK_IF;
  else if (kw_is_equal(start, "else", length)) return TOK_ELSE;
  else if (kw_is_equal(start, "true", length)) return TOK_TRUE;
  else if (kw_is_equal(start, "false", length)) return TOK_FALSE;
  else if (kw_is_equal(start, "func", length)) return TOK_FUNC;
  else if (kw_is_equal(start, "for", length)) return TOK_FOR;
  else if (kw_is_equal(start, "null", length)) return TOK_NULL;
  else if (kw_is_equal(start, "return", length)) return TOK_RETURN;
  return TOK_IDENTIFIER;
}

void lexer_begin(LexerResult *result, const char *source, Arena *arena) {
  TokenArray *array = malloc(sizeof(TokenArray)); // free called in program.c
  result->tokens = array;
  result->errors = malloc(sizeof(LogArray)); // free called in program.c

  LexerContext context = {
    .source = source,
    .tokens = array,
    .errors = result->errors,
    .arena = arena,
    .current = 0,
    .line = 0
  };

  da_init(array, 32);
  da_init(result->errors, 8);

  while (*peek(&context) != '\0') {
    switch (*peek(&context)) {
    case '(': add_token(&context, TOK_LEFT_PAREN, advance(&context), 1); break;
    case ')': add_token(&context, TOK_RIGHT_PAREN, advance(&context), 1); break;
    case '{': add_token(&context, TOK_LEFT_BRACE, advance(&context), 1); break;
    case '}': add_token(&context, TOK_RIGHT_BRACE, advance(&context), 1); break;
    case ',': add_token(&context, TOK_COMMA, advance(&context), 1); break;
    case '.': add_token(&context, TOK_DOT, advance(&context), 1); break;
    case '+': add_token(&context, TOK_PLUS, advance(&context), 1); break;
    case '-': add_token(&context, TOK_MINUS, advance(&context), 1); break;
    case '*': add_token(&context, TOK_STAR, advance(&context), 1); break;
    case ':': add_token(&context, TOK_COLON, advance(&context), 1); break;
    case ';': add_token(&context, TOK_SEMICOLON, advance(&context), 1); break;
    case '/': {
      if (*peek_next(&context) == '/') {
        seek(&context, '\n');
      } else {
        add_token(&context, TOK_SLASH, advance(&context), 1);
      }
      break;
    }
    case '=': {
      bool matches = *peek_next(&context) == '=';
      add_token(&context, matches ? TOK_DOUBLE_EQUAL : TOK_EQUAL, advance(&context), matches ? 2 : 1);
      if (matches) advance(&context);
      break;
    }
    case '>': {
      bool matches = *peek_next(&context) == '=';
      add_token(&context, matches ? TOK_GREATER_EQUAL : TOK_GREATER, advance(&context), matches ? 2 : 1);
      if (matches) advance(&context);
      break;
    }
    case '<': {
      bool matches = *peek_next(&context) == '=';
      add_token(&context, matches ? TOK_LESS_EQUAL : TOK_LESS, advance(&context), matches ? 2 : 1);
      if (matches) advance(&context);
      break;
    }
    case '!': {
      bool matches = *peek_next(&context) == '=';
      add_token(&context, matches ? TOK_BANG_EQUAL : TOK_BANG, advance(&context), matches ? 2 : 1);
      if (matches) advance(&context);
      break;
    }
    case '"': {
      advance(&context);
      u32 start = context.current;
      if (seek(&context, '"')) {
        u32 length = context.current - start;
        add_token(&context, TOK_STRING, &source[start], length);
        advance(&context);
      }
      break;
    }
    case ' ':
    case '\r':
    case '\t':
      advance(&context);
      break;
    case '\n':
      context.line++;
      advance(&context);
      break;
    default: {
      if (isdigit(*peek(&context))) {
        u32 start = context.current;

        while (isdigit(*peek(&context))) advance(&context);

        if (*peek(&context) == '.') {
          if (isdigit(*peek_next(&context))) {
            advance(&context);
            while (isdigit(*peek(&context))) advance(&context);
          } else {
            log_err(context.errors, context.line + 1, "Unexpected number with trailing dot");
            advance(&context);
          }
        }

        u32 length = context.current - start;
        add_token(&context, TOK_NUMBER, &source[start], length);

        break;
      } else if (isalpha(*peek(&context))) {
        u32 start = context.current;
        while (isalpha(*peek(&context))) advance(&context);
        u32 length = context.current - start;
        TokenKind kind = str_to_identifier_kind(&source[start], length);
        add_token(&context, kind, &source[start], length);
      } else {
        char *ptr = arena_alloc_format(context.arena, "Unexpected character '%c'", *peek(&context));
        log_err(context.errors, context.line + 1, ptr);
        advance(&context);
      }
    }
    }
  }

  add_token(&context, TOK_EOF, advance(&context), 1);
}

void print_token_array(TokenArray *array) {
  for (u32 i = 0; i < array->size; i++) {
    printf("%s " SV_FMT " (line %d)\n", token_to_str(array->items[i]), SV_ARG(array->items[i].lexeme), array->items[i].line);
  }
}
