#ifndef __LANG_OBJECTS_H__
#define __LANG_OBJECTS_H__

#include "lang_value.h"

struct _htable;

typedef enum {
  OP_NONE = 0,
  OP_RETURN,
  OP_LOAD_CONST,
  OP_LOAD,
  OP_ADD,
} opCode;

static char *op2str[] =
{
  "NONE",
  "RETURN",
  "LOAD_CONST",
  "LOAD",
  "ADD"
};

#define NUM_OPCODES ((int)OP_RETURN + 1)

typedef struct __attribute__((__packed__)) _operation {
  uint32_t op;
  uint32_t a, b, r;
} operation;

/**
 * Functions
 */
typedef struct _function {
  /* Static part of function created at compilation time */
  size_t codesz;
  operation *code;
  uint8_t *binary;
  size_t binarysz;
  value *k;
  size_t ksz;
  size_t stacksz;
  struct _htable *vars;
} function;

typedef struct _call {
  function *f;
  value *stack;
} call;

static inline void free_function(function *f)
{
  if (f->code) free(f->code);
  if (f->binary) free(f->binary);
  if (f->k) free(f->k);
  if (f->vars) free(f->vars);
  free(f);
}

static inline void free_call(call *c)
{
  if (c->stack) free(c->stack);
  free(c);
}

#endif
