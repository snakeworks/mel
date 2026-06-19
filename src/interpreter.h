#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "base.h"
#include "parser.h"

typedef struct {
  i32 exit_code;
} InterpreterResult;

void value_print(Value value);
void interpreter_begin(InterpreterResult *result, StmtArray *statements);

#endif
