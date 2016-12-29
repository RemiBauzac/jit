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

  /* Alloc stack now */
  fread(&main->stacksz, sizeof(size_t), 1, bcf);
  if (main->stacksz > 0) {
    main->stack = malloc(sizeof(value)*main->stacksz);
    if (!main->stack) {
      fprintf(stderr, "Cannot allocate stack\n");
      fclose(bcf);
      free(main->code);
      free(main->k);
      free(main);
      return NULL;
    }
  }
  return main;
}

static int64_t interpret(function *fmain)
{
  int pc;
  int64_t ret = 0;
  operation op;
  value *a, *b, *r;

  /* walking through main function */
  for(pc = 0; pc < fmain->codesz; pc++) {
    op = fmain->code[pc];
    switch(op.op) {
      /* On return, get the return value from parameters and exit */
      case OP_RETURN:
        a = VALUEA(fmain, op);
        ret = ivalue(*a);
        break;
      case OP_LOAD:
        a = VALUEA(fmain, op);
        r = VALUER(fmain, op);
        ivalue(*r) = ivalue(*a);
        break;
      case OP_ADD:
        a = VALUEA(fmain, op);
        b = VALUEB(fmain, op);
        r = VALUER(fmain, op);
        ivalue(*r) = ivalue(*a) + ivalue(*b);
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
    /* start interpreter, get return value and print it */
    ret = interpret(fmain);
  }
  fprintf(stdout, "Return value is %lld\n", ret);

  /* cleanly exit from interpreter */
  free_function(fmain);
  exit(0);
}
