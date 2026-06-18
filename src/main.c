#include "base.h"
#include "program.h"

i32 main(void) {
  const char *sample_source = "10 + 5\n";

  program_run(sample_source);

  return 0;
}
