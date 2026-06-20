#include "native.h"

Value native_print(Value *args, u32 count) {
  if (count < 1) {
    return NULL_VALUE;
  }

  for (u32 i = 0; i < count; i++) {
    print_value(args[i]);
  }

  return NULL_VALUE;
}

NativeFn map_identifier_to_native_fn(StringView identifier) {
  if (sv_is_equal_to_cstr(identifier, "print")) return native_print;
  return NULL;
}
