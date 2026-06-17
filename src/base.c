#include "base.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

void log_err(u32 line, const char *msg, ...) {
  va_list args;
  printf("[ERR] ");
  va_start(args, msg);
  vprintf(msg, args);
  printf(" (line %d)\n", line);
  va_end(args);
}

void log_fatal(u32 line, const char *msg, ...) {
  va_list args;
  printf("[FATAL] ");
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  printf(" (line %d)\n", line);
  exit(1);
}

bool sv_is_equal(StringView sv1, StringView sv2) {
  if (sv1.length != sv2.length) return false;
  return memcmp(sv1.start, sv2.start, sv1.length) == 0;
}

f64 sv_to_f64(StringView sv) {
  char cstr[sv.length + 1];
  strncpy(cstr, sv.start, sv.length);
  cstr[sv.length] = '\0';
  return atof(cstr);
}
