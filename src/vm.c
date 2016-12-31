#include "lang.h"
#include "jit/jit.h"

static function *load_main(const char *filename)
{
  function *main = NULL;
  FILE *bcf;
  int cookie = 0, version = 0;
  size_t codesz, ksz;

  /* Open file */
  bcf = fopen(filename, "rb");
  if (!bcf) {
    fprintf(stderr, "Cannot open %s bytecode\n", filename);
    return NULL;
  }

  /* Check cookie */
  fread(&cookie, sizeof(char), 4, bcf);
  if (cookie != LANG_COOKIE) {
    fprintf(stderr, "Bad bytecode file format\n");
    fclose(bcf);
    return NULL;
  }

  /**
   * Check version.
   * Useless now, but it will be helpful later
   */
  fread(&version, sizeof(char), 4, bcf);
  if (version != LANG_VERSION) {
    fprintf(stderr, "Bad lang version %d. %d was awaited\n", version, LANG_VERSION);
    fclose(bcf);
    return NULL;
  }

  /* Alloc main function structure */
  main = malloc(sizeof(function));
  if (!main) return NULL;
  memset(main, 0, sizeof(function));

  /* size from file */
  fread(&main->codesz, sizeof(size_t), 1, bcf);
  if (main->codesz == 0) {
    fprintf(stderr, "Empty bytecode file\n");
    fclose(bcf);
    return main;
  }

  /* Alloc code regarding code size */
  main->code = malloc(sizeof(operation)*main->codesz);
  if (!main->code) {
    fclose(bcf);
    free(main);
    return NULL;
  }

  /* Put code to structure */
  codesz = fread(main->code, sizeof(operation), main->codesz, bcf);
  if (codesz != main->codesz) {
    fprintf(stderr, "Error reading bytecode in file\n");
    fclose(bcf);
    free(main->code);
    free(main);
    return NULL;
  }

  /* Read constants size from file */
  fread(&main->ksz, sizeof(size_t), 1, bcf);

  /* Alloc constants regarding constant size */
  main->k = malloc(sizeof(value)*main->ksz);
  if (!main->k) {
    fprintf(stderr, "Error allocating constants\n");
    fclose(bcf);
    free(main->code);
    free(main);
    return NULL;
  }
  ksz = fread(main->k, sizeof(value), main->ksz, bcf);
  if (ksz != main->ksz) {
    fprintf(stderr, "Error reading constants in file\n");
    fclose(bcf);
    free(main->code);
    free(main->k);
    free(main);
    return NULL;
  }

  return main;
}

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
        c->stack[op.r] = c->stack[op.a];
        break;
      case OP_ADD:
        setivalue(c->stack[op.r],
            ivalue(c->stack[op.a]) + ivalue(c->stack[op.b]))
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

int main(int argc, char **argv)
{
  function *fmain;
  int64_t ret = 0;
  int jit = 0;
  char *filename;

  if (argc == 3 && !strcmp(argv[1], "-j")) {
    jit = 1;
    filename = argv[2];
  }
  else if (argc == 2) {
    filename = argv[1];
  }
  else {
    fprintf(stderr, "usage: %s [-j] <file>\n", argv[0]);
    exit(1);
  }
  /* load main function, exit on error */
  fmain = load_main(filename);
  if (!fmain) exit(ENOMEM);

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
