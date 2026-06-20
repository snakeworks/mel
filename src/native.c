#include "native.h"
#include "parser.h"

Value native_print(Value *args, u32 count) {
  if (count < 1) {
    return NULL_VALUE;
  }

  for (u32 i = 0; i < count; i++) {
    print_value(args[i]);
  }

  return NULL_VALUE;
}

Value native_min(Value *args, u32 count) {
  if (count != 2) {
    return NULL_VALUE;
  }

  if (args[0].type != TYPE_NUMBER || args[1].type != TYPE_NUMBER) {
    return NULL_VALUE;
  }

  return args[0].as.number > args[1].as.number ? args[1] : args[0];
}

Value native_max(Value *args, u32 count) {
  if (count != 2) {
    return NULL_VALUE;
  }

  if (args[0].type != TYPE_NUMBER || args[1].type != TYPE_NUMBER) {
    return NULL_VALUE;
  }

  return args[0].as.number > args[1].as.number ? args[0] : args[1];
}

NativeFn map_identifier_to_native_fn(StringView identifier) {
  if (sv_is_equal_to_cstr(identifier, "print")) return native_print;
  else if (sv_is_equal_to_cstr(identifier, "min")) return native_min;
  else if (sv_is_equal_to_cstr(identifier, "max")) return native_max;
  return NULL;
}
