#ifndef __LANG_OP_H__
#define __LANG_OP_H__

typedef enum {
  OP_NONE = 0,
  OP_RETURN,
  OP_LOAD_CONST,
  OP_LOAD,
  OP_ADD,
  OP_SUB,
  OP_MUL,
} opCode;

static char *op2str[] =
{
  "NONE",
  "RETURN",
  "LOAD_CONST",
  "LOAD",
  "ADD"
  "SUB",
  "MUL"
};

#define NUM_OPCODES ((int)OP_RETURN + 1)

typedef struct __attribute__((__packed__)) _operation {
  uint32_t op;
  uint32_t a, b, r;
} operation;



#endif
