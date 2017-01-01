#include "marshal.h"

static void marshal_function(FILE *out, function *f)
{
  if (!out || !f) return;
  fwrite(&f->codesz, sizeof(size_t), 1, out);
  fwrite(f->code, sizeof(operation), f->codesz, out);
  fwrite(&f->ksz, sizeof(size_t), 1, out);
  fwrite(f->k, sizeof(value), f->ksz, out);
  fwrite(&f->stacksz, sizeof(size_t), 1, out);
}

void marshal(FILE *out, function *f)
{
  uint32_t cookie = LANG_COOKIE, version = LANG_VERSION;
  if (!out || !f) return;

  fwrite(&cookie,  sizeof(char), sizeof(uint32_t), out);
  fwrite(&version, sizeof(char), sizeof(uint32_t), out);
  marshal_function(out, f);
}


static function *unmarshal_function(FILE *in)
{
  function *f = NULL;
  size_t codesz, ksz;

  /* Alloc function structure */
  f = malloc(sizeof(function));
  if (!f) return NULL;
  memset(f, 0, sizeof(function));

  /* size from file */
  fread(&f->codesz, sizeof(size_t), 1, in);
  if (f->codesz == 0) return f;

  /* Alloc code regarding code size */
  f->code = malloc(sizeof(operation)*f->codesz);
  if (!f->code) {
    fprintf(stderr, "Error allocate bytecode\n");
    free(f);
    return NULL;
  }

  /* Put code to structure */
  codesz = fread(f->code, sizeof(operation), f->codesz, in);
  if (codesz != f->codesz) {
    fprintf(stderr, "Error reading bytecode in file\n");
    free(f->code);
    free(f);
    return NULL;
  }

  /* Read constants size from file */
  fread(&f->ksz, sizeof(size_t), 1, in);

  /* Alloc constants regarding constant size */
  f->k = malloc(sizeof(value)*f->ksz);
  if (!f->k) {
    fprintf(stderr, "Error allocating constants\n");
    free(f->code);
    free(f);
    return NULL;
  }
  ksz = fread(f->k, sizeof(value), f->ksz, in);
  if (ksz != f->ksz) {
    fprintf(stderr, "Error reading constants in file\n");
    free(f->code);
    free(f->k);
    free(f);
    return NULL;
  }

  /* finally read stack size */
  fread(&f->stacksz, sizeof(size_t), 1, in);

  return f;
}

function *unmarshal(char *filename)
{
  function *f = NULL;
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
  return unmarshal_function(bcf);
}

