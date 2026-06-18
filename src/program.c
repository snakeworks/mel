#include "program.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

void program_run(const char *source) {
  Arena *arena = arena_init(1024 * 1000); // 1 MB
  LexerResult lexer_result = {0};
  ParserResult parser_result = {0};

  lexer_begin(&lexer_result, source, arena);

  if (lexer_result.errors->size > 0) {
    print_logs(lexer_result.errors);
    goto cleanup;
    return;
  }

  parser_begin(&parser_result, lexer_result.tokens, arena);

  if (parser_result.errors->size > 0) {
    print_logs(parser_result.errors);
    goto cleanup;
    return;
  }

  // TODO: Temporary, remove later
  Value value = expr_eval(parser_result.expr);
  value_print(value); printf("\n");

cleanup:
  if (arena != NULL) arena_free(arena);
  if (lexer_result.tokens != NULL) free(lexer_result.tokens);
  if (lexer_result.errors != NULL) free(lexer_result.errors);
  if (parser_result.errors != NULL) free(parser_result.errors);
}
