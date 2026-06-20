#include "program.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <stdio.h>

void program_run(const char *source) {
  Arena *arena = arena_init(1024 * 1000); // 1 MB
  LexerResult lexer_result = {0};
  ParserResult parser_result = {0};
  InterpreterResult interpreter_result = {0};

  lexer_begin(&lexer_result, source, arena);

  if (lexer_result.errors->size > 0) {
    print_logs(lexer_result.errors);
    goto cleanup;
  }

  parser_begin(&parser_result, lexer_result.tokens, arena);

  if (parser_result.errors->size > 0) {
    print_logs(parser_result.errors);
    goto cleanup;
  }

  print_stmt_array(parser_result.statements, 0);

  interpreter_begin(&interpreter_result, parser_result.statements);

cleanup:
  if (arena != NULL) arena_free(arena);
  if (lexer_result.tokens != NULL) { da_free(lexer_result.tokens); free(lexer_result.tokens); }
  if (lexer_result.errors != NULL) { da_free(lexer_result.errors); free(lexer_result.errors); }
  if (parser_result.statements != NULL) { da_free(parser_result.statements); free(parser_result.statements); }
  if (parser_result.errors != NULL) { da_free(parser_result.errors); free(parser_result.errors); }
}
