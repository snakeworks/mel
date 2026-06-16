#include "base.h"
#include <string.h>

bool sv_is_equal(StringView sv1, StringView sv2) {
  char cstr1[sv1.length + 1];
  char cstr2[sv2.length + 2];

  strncpy(cstr1, sv1.start, sv1.length);
  strncpy(cstr2, sv2.start, sv2.length);
  cstr1[sv1.length] = '\0';
  cstr2[sv2.length] = '\0';

  return strcmp(cstr1, cstr2) == 0;
}

f64 sv_to_f64(StringView sv) {
  char cstr[sv.length + 1];
  strncpy(cstr, sv.start, sv.length);
  cstr[sv.length] = '\0';
  return atof(cstr);
}
