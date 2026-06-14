#include "base.h"
#include "lexer.h"

i32 main(void) {
  TokenArray tokens;

  const char *sample_source =
    "// this is a comment\n"
    "(( )){} // grouping stuff\n"
    "!*+-/=<> <= >= != == // operators\n"
    "\"this is my string\"\n"
    "42 52.0124 19834009\n"
    "for if testingg null\n";

  tokenize(&tokens, sample_source);

  print_token_array(&tokens);

  return 0;
}
