#ifndef BASE_H_
#define BASE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
  const char *start;
  u32 length;
} StringView;

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int) (sv).length, (sv).start

#endif
