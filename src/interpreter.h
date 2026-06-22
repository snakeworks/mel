#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "base.h"
#include "parser.h"

typedef struct {
  i32 exit_code;
} InterpreterResult;

typedef struct {
  StringView identifier;
  Value value;
} Binding;

typedef struct {
  Binding *items;
  u32 capacity;
  u32 size;
} BindingArray;

typedef struct Environment Environment;

struct Environment {
  BindingArray *bindings;
  Environment *parent;
};

typedef struct {
  StmtArray *statements;
  Arena *arena;
  Environment *cur_env;
  LogArray *errors;
} InterpreterContext;

void interpreter_begin(InterpreterResult *result, StmtArray *statements);

#endif
