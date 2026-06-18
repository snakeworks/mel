#ifndef BASE_H_
#define BASE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#define f64 double

#define da_init(array, initial_capacity)                                       \
  array->items = malloc(initial_capacity * sizeof(*array->items));             \
  array->size = 0;                                                             \
  array->capacity = initial_capacity;

#define da_append(array, element)                                              \
  do {                                                                         \
    if (array->size >= array->capacity) {                                      \
      array->capacity *= 2;                                                    \
      array->items =                                                           \
          realloc(array->items, array->capacity * sizeof(*array->items));      \
    }                                                                          \
    array->items[array->size++] = element;                                     \
  } while (false);

#define CASE_STRING(val)                                                       \
  case val:                                                                    \
    return #val

typedef struct {
  u64 capacity;
  u64 offset;
} Arena;

typedef struct {
  const char *start;
  u32 length;
} StringView;

typedef enum {
  LOG_ERR,
  LOG_FATAL
} LogLevel;

typedef struct {
  LogLevel level;
  const char *msg;
  u32 line;
} LogMessage;

typedef struct {
  LogMessage *items;
  u32 size;
  u32 capacity;
} LogArray;

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int) (sv).length, (sv).start

#define arena_push(arena, T) ((T *)arena_alloc(arena, sizeof(T), _Alignof(T)))

Arena *arena_init(u64 size);
void arena_free(Arena *arena);
void *arena_alloc(Arena *arena, u64 size, u64 align);

void log_err(LogArray *logs, u32 line, const char *msg, ...);
void vlog_err(LogArray *array, u32 line, const char *msg, va_list args);
void log_fatal(LogArray *logs, u32 line, const char *msg, ...);
void vlog_fatal(LogArray *array, u32 line, const char *msg, va_list args);
void print_logs(LogArray *logs);

bool sv_is_equal(StringView sv1, StringView sv2);
f64 sv_to_f64(StringView sv);

#endif
