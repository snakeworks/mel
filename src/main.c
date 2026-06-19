#include "base.h"
#include "program.h"

i32 main(void) {
  const char *sample_source = "1 + 52; { 10 + 2.5; 5 + 1; }";

  program_run(sample_source);

  return 0;
}
