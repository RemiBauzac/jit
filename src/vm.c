#include "lang.h"
#include "marshal.h"
#include "jit/jit.h"

static int64_t interpret(call *c)
{
  int pc;
  int64_t ret = 0;
  function *f;
  operation op;
  value *a, *b, *r;

  if (!c) return ret;
  f = c->f;

  /* walking through opcodes */
  for(pc = 0; pc < f->codesz; pc++) {
    op = f->code[pc];
    switch(op.op) {
      /* On return, get the return value from parameters and exit */
      case OP_RETURN:
        return ivalue(c->stack[op.r]);
        break;
      case OP_LOAD_CONST:
        c->stack[op.r] = f->k[op.a];
        break;
      case OP_LOAD:
        fprintf(stdout, "OP_LOAD: stack: %p, r:%d, a:%d\n", c->stack, op.r, op.a);
        c->stack[op.r] = c->stack[op.a];
        break;
      case OP_ADD:
        setivalue(c->stack[op.r],
            ivalue(c->stack[op.a]) + ivalue(c->stack[op.b]))
        break;
      case OP_SUB:
        setivalue(c->stack[op.r],
            ivalue(c->stack[op.a]) - ivalue(c->stack[op.b]))
        break;
      case OP_MUL:
        setivalue(c->stack[op.r],
            ivalue(c->stack[op.a]) * ivalue(c->stack[op.b]))
        break;
      /* Handle error case */
      case OP_NONE:
      default:
        fprintf(stderr, "Unavailable operand %u\n", op.op);
        exit(1);
    }
  }
  return ret;
}

static void dump_function(function *f)
{

  int pc;
  operation op;

  for(pc = 0; pc < f->codesz; pc++) {
    op = f->code[pc];
    fprintf(stdout, "%d: %s %u %u %u\n", pc, op2str[op.op], op.r, op.a, op.b);
  }
}


int main(int argc, char **argv)
{
  function *fmain;
  int64_t ret = 0;
  int jit = 0, dump = 0;
  char *filename;

  if (argc == 3 && !strcmp(argv[1], "-j")) {
    jit = 1;
    filename = argv[2];
  }
  else if (argc == 3 && !strcmp(argv[1], "-d")) {
    dump = 1;
    filename = argv[2];
  }
  else if (argc == 2) {
    filename = argv[1];
  }
  else {
    fprintf(stderr, "usage: %s [-j|-d] <file>\n", argv[0]);
    exit(1);
  }

  /* load main function, exit on error */
  fmain = unmarshal(filename);
  if (!fmain) exit(ENOMEM);

  if (dump) {
    dump_function(fmain);
  }

  /* create executable */
  if (jit) {
    create_binary(fmain);
  }
  if (fmain->binary) {
    /* cast binary into function pointer */
    uint64_t (*execute)(void) = (void *)fmain->binary;
    /* and call this function */
    ret = execute();
  }
  else {
    /* create a call */
    call *ci = malloc(sizeof(call));
    if (!ci) {
      free_function(fmain);
      exit(ENOMEM);
    }
    ci->f = fmain;
    ci->stack = malloc(sizeof(value)*fmain->stacksz);
    if (!ci->stack) {
      free_function(fmain);
      exit(ENOMEM);
    }
    /* start interpreter, get return value and print it */
    ret = interpret(ci);
    free_call(ci);
  }
  fprintf(stdout, "Return value is %lld\n", ret);
  free_function(fmain);

  /* cleanly exit from interpreter */
  exit(0);
}
