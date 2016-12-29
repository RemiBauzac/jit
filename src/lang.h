#ifndef __OP_H__
#define __OP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define LANG_COOKIE  0xdeadbeef
#define LANG_VERSION 0x00000002 /* mean 0.2 */


/* variables types */
#define TYPE_NONE 0
#define TYPE_INT  1
#define TYPE_FLT  2

typedef struct _value {
  uint32_t type;
  union {
    void *ptr;
    int64_t integer;
    double   flt;
  }vu;
} value;

#define ivalue(v) (v).vu.integer
#define fvalue(v) (v).vu.flt

#define is_i(v) ((v).type == TYPE_INT)
#define is_f(v) ((v).type == TYPE_FLT)

typedef enum {
  OP_NONE = 0,
  OP_RETURN,
  OP_LOAD,
  OP_ADD,
} opCode;

static value none = {
  .type = TYPE_NONE,
  .vu.ptr = NULL
};


#define NUM_OPCODES ((int)OP_RETURN + 1)

typedef struct __attribute__((__packed__)) _operation {
  uint32_t op;
  uint8_t wa, wb, wr, __reserved1;
  uint32_t a, b, r;
  uint32_t __reserved2;
} operation;

/* variable space */
#define LANG_NONE  0
#define LANG_CONST 1
#define LANG_VAR   2

#define IS_ANONE(op) (op.wa == LANG_NONE)
#define IS_BNONE(op) (op.wb == LANG_NONE)
#define IS_RNONE(op) (op.wr == LANG_NONE)

#define IS_ACONST(op) (op.wa == LANG_CONST)
#define IS_BCONST(op) (op.wb == LANG_CONST)
#define IS_RCONST(op) (op.wr == LANG_CONST)

#define IS_AVAR(op) (op.wa == LANG_VAR)
#define IS_BVAR(op) (op.wb == LANG_VAR)
#define IS_RVAR(op) (op.wr == LANG_VAR)


#define VALUEA(f, op) (IS_ACONST(op)?&f->k[op.a]:IS_AVAR(op)?&f->stack[op.a]:&none)
#define VALUEB(f, op) (IS_BCONST(op)?&f->k[op.b]:IS_BVAR(op)?&f->stack[op.b]:&none)
#define VALUER(f, op) (IS_RCONST(op)?&f->k[op.r]:IS_RVAR(op)?&f->stack[op.r]:&none)


typedef struct _function {
  /* Static part of function created at compilation time */
  size_t codesz;
  operation *code;
  uint8_t *binary;
  size_t binarysz;
  value *k;
  size_t ksz;
  size_t stacksz;
  const char **vars;

  value *stack;
} function;

static inline void free_function(function *f)
{
  if (f->code) free(f->code);
  if (f->binary) free(f->binary);
  if (f->k) free(f->k);
  if (f->vars) free(f->vars);
  if (f->stack) free(f->stack);
  free(f);
}


#endif
