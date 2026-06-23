#include "native.h"
#include "parser.h"
#include <stdio.h>

Value native_print(Value *args, u32 count) {
  if (count < 1) {
    return NULL_VALUE;
  }

  for (u32 i = 0; i < count; i++) {
    print_value(args[i]);
  }

  return NULL_VALUE;
}

Value native_println(Value *args, u32 count) {
  if (count < 1) {
    return NULL_VALUE;
  }

  for (u32 i = 0; i < count; i++) {
    print_value(args[i]); printf("\n");
  }

  return NULL_VALUE;
}

Value native_min(Value *args, u32 count) {
  if (count != 2) {
    return NULL_VALUE;
  }

  bool l_num = (args[0].type == TYPE_INT || args[0].type == TYPE_FLOAT);
  bool r_num = (args[1].type == TYPE_INT || args[1].type == TYPE_FLOAT);

  if (!l_num || !r_num) {
    return NULL_VALUE;
  }

  if (args[0].type == TYPE_INT && args[1].type == TYPE_INT) {
    i64 a = args[0].as.integer, b = args[1].as.integer;
    return a > b ? args[1] : args[0];
  } else {
    f64 a = (args[0].type == TYPE_INT) ? (f64)args[0].as.integer : args[0].as.floating_point;
    f64 b = (args[1].type == TYPE_INT) ? (f64)args[1].as.integer : args[1].as.floating_point;
    return a > b ? args[1] : args[0];
  }
}

Value native_max(Value *args, u32 count) {
  if (count != 2) {
    return NULL_VALUE;
  }

  bool l_num = (args[0].type == TYPE_INT || args[0].type == TYPE_FLOAT);
  bool r_num = (args[1].type == TYPE_INT || args[1].type == TYPE_FLOAT);

  if (!l_num || !r_num) {
    return NULL_VALUE;
  }

  if (args[0].type == TYPE_INT && args[1].type == TYPE_INT) {
    i64 a = args[0].as.integer, b = args[1].as.integer;
    return a > b ? args[0] : args[1];
  } else {
    f64 a = (args[0].type == TYPE_INT) ? (f64)args[0].as.integer : args[0].as.floating_point;
    f64 b = (args[1].type == TYPE_INT) ? (f64)args[1].as.integer : args[1].as.floating_point;
    return a > b ? args[0] : args[1];
  }
}

Value native_array_append(Value *args, u32 count) {
  if (count != 2) {
    return NULL_VALUE;
  }

  if (args[0].type != TYPE_ARRAY) {
    return NULL_VALUE;
  }

  Expr *e = malloc(sizeof(Expr));
  e->kind = EXPR_VALUE;
  e->as.value = args[1];

  da_append(args[0].as.array, e);

  return NULL_VALUE;
}

Value native_array_size(Value *args, u32 count) {
  if (count != 1) {
    return NULL_VALUE;
  }

  if (args[0].type != TYPE_ARRAY) {
    return NULL_VALUE;
  }

  return (Value){
    .type = TYPE_INT,
    .as.integer = args[0].as.array->size
  };
}

NativeFn map_identifier_to_native_fn(StringView identifier) {
  if (sv_is_equal_to_cstr(identifier, "print")) return native_print;
  else if (sv_is_equal_to_cstr(identifier, "println")) return native_println;
  else if (sv_is_equal_to_cstr(identifier, "min")) return native_min;
  else if (sv_is_equal_to_cstr(identifier, "max")) return native_max;
  else if (sv_is_equal_to_cstr(identifier, "array_append")) return native_array_append;
  else if (sv_is_equal_to_cstr(identifier, "array_size")) return native_array_size;
  return NULL;
}
