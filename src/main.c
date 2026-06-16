#include "base.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

i32 main(void) {
  const char *sample_source = "1 * (2 - 1)";

  TokenArray tokens;
  tokenize(&tokens, sample_source);
  ParseContext context = {.tokens = &tokens, .current = 0};
  Expr *e = parser_start(&context);
  //expr_print(e); printf("\n");
  f64 result = expr_eval(e);
  printf("Result: %g\n", result);

  return 0;
}
