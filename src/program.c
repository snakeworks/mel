#include "program.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

void program_run(const char *source) {
  Arena *arena = arena_init(1024 * 1000); // 1 MB

  LexerResult lexer_result = {0};
  lexer_begin(&lexer_result, source, arena);

  if (lexer_result.errors->size > 0) {
    print_logs(lexer_result.errors);
    goto cleanup;
    return;
  }

  ParserResult parser_result = {0};
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
  arena_free(arena);
  free(lexer_result.tokens);
  free(lexer_result.errors);
  free(parser_result.errors);
}
