#include "base.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

i32 main(void) {
  const char *sample_source = "(2 + 3) * 4\n";

  TokenArray tokens;
  tokenize(&tokens, sample_source);
  print_token_array(&tokens);
  ParseContext context = {.tokens = &tokens, .current = 0};
  Expr *e = expr_parse(&context);
  expr_print(e); printf("\n");
  Value result = expr_eval(e);
  printf("Result: "); value_print(result); printf("\n");

  return 0;
}
