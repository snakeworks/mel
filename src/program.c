#include "program.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

void program_run(const char *source) {
  LexerResult lexer_result = {0};
  lexer_begin(&lexer_result, source);

  if (lexer_result.errors->size > 0) {
    print_logs(lexer_result.errors);
    return;
  }

  ParserResult parser_result = {0};
  parser_begin(&parser_result, lexer_result.tokens);

  if (parser_result.errors->size > 0) {
    print_logs(parser_result.errors);
    return;
  }

  // TODO: Temporary, remove later
  Value value = expr_eval(parser_result.expr);
  value_print(value); printf("\n");
}
