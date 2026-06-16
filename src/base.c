#include "base.h"
#include <string.h>

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
