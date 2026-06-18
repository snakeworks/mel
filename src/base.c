#include "base.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static u64 align_up(u64 n, u64 align) {
  assert((align & (align - 1)) == 0);
  return (n + (align - 1)) & ~(align - 1);
}

Arena *arena_init(u64 capacity) {
  Arena *arena = (Arena *)malloc(capacity + sizeof(Arena));
  if (arena == NULL) {
    // program ran out of memory, returning NULL here, because we
    // have bigger problems to worry about
    return NULL;
  }
  arena->capacity = capacity + sizeof(Arena);
  arena->offset = sizeof(Arena);
  return arena;
}

void arena_free(Arena *arena) {
  if (arena == NULL) {
    return;
  }
  free(arena);
}

void *arena_alloc(Arena *arena, u64 size, u64 align) {
  u64 aligned = align_up(arena->offset, align);
  assert(aligned <= arena->capacity && size <= arena->capacity - aligned && "arena exhausted");
  void *ptr = (u8*)arena + aligned;
  arena->offset = aligned + size;
  return ptr;
}

void *arena_alloc_format(Arena *arena, const char *format, ...) {
  va_list args;
  va_start(args, format);
  void *ptr = arena_alloc_vformat(arena, format, args);
  va_end(args);
  return ptr;
}

void *arena_alloc_vformat(Arena *arena, const char *format, va_list args) {
  va_list copy;
  va_copy(copy, args);

  i32 len = vsnprintf(NULL, 0, format, copy);
  assert(len >= 0 && "arena_alloc_vformat formatting failed");

  va_end(copy);

  char *buf = arena_alloc(
    arena,
    len + 1,
    _Alignof(char)
  );

  vsnprintf(buf, len + 1, format, args);
  buf[len] = '\0';

  return buf;
}

void log_err(LogArray *array, u32 line, const char *msg) {
  LogMessage m = { .level = LOG_ERR, .msg = msg, .line = line };
  da_append(array, m);
}

void log_fatal(LogArray *array, u32 line, const char *msg) {
  LogMessage m = { .level = LOG_FATAL, .msg = msg, .line = line };
  da_append(array, m);
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
      "%s: %s (line %d)\n",
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
