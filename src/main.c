#include "base.h"
#include "program.h"
#include <stdio.h>

void print_usage() {
  printf(
    "Usage:\n"
    "  mel <filepath>\n"
  );
}

char *read_entire_file(const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (f == NULL) {
    printf("Failed to open file\n");
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  u64 fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *dest = malloc(fsize + 1);
  fread(dest, fsize, sizeof(char), f);
  fclose(f);

  return dest;
}

i32 main(i32 argc, char **argv) {
  if (argc <= 1) {
    print_usage();
    return 0;
  }

  char *source = read_entire_file(argv[1]);
  program_run(source);

  return 0;
}
