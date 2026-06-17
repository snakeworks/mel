#include "base.h"
#include <string.h>
#include <stdio.h>

// TODO: As you can see, memory leaks are none of my concern at the moment, but please fix this later
static char *vformat(const char *msg, va_list args) {
  va_list copy;
  va_copy(copy, args);
  u32 len = vsnprintf(NULL, 0, msg, copy);
  va_end(copy);

  char *buf = malloc(len + 1);
  vsnprintf(buf, len + 1, msg, args);
  return buf;
}

void vlog_err(LogArray *array, u32 line, const char *msg, va_list args) {
  char *formatted = vformat(msg, args);
  LogMessage m = { .level = LOG_ERR, .msg = formatted, .line = line };
  da_append(array, m);
}

void log_err(LogArray *array, u32 line, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vlog_err(array, line, msg, args);
  va_end(args);
}

void vlog_fatal(LogArray *array, u32 line, const char *msg, va_list args) {
  char *formatted = vformat(msg, args);
  LogMessage m = { .level = LOG_FATAL, .msg = formatted, .line = line };
  da_append(array, m);
}

void log_fatal(LogArray *array, u32 line, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vlog_fatal(array, line, msg, args);
  va_end(args);
}

const char *log_level_to_str(LogLevel level) {
  switch (level) {
  case LOG_ERR: return "ERR";
  case LOG_FATAL: return "FATAL";
  }
  return "";
}

void print_logs(LogArray *logs) {
  for (u32 i = 0; i < logs->size; i++) {
    printf(
      "[%s] %s (line %d)",
      log_level_to_str(logs->items[i].level),
      logs->items[i].msg,
      logs->items[i].line
    );
  }
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
