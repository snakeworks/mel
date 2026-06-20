#ifndef NATIVE_H_
#define NATIVE_H_

#include "base.h"
#include "parser.h"

typedef Value (*NativeFn)(Value *args, u32 count);

NativeFn map_identifier_to_native_fn(StringView identifier);

#endif
