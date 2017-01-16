#ifndef __LANG_FUNCTION_H__
#define __LANG_FUNCTION_H__

#include "lang_value.h"
#include "lang_op.h"
struct _htable;

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
