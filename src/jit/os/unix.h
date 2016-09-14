#ifndef __MACOS_H__
#define __MACOS_H__

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <unistd.h>
#include "../jit.h"

static inline void bin_free(function *f)
{
  int tofree = f->binarysz;
  int pagesz = getpagesize();

  if (!f->binary) {
    return;
  }

  if (f->binarysz % pagesz) {
    tofree = f->binarysz + (pagesz - f->binarysz%pagesz);
  }

  munmap(f->binary, tofree);
  f->binary = NULL;
}

static inline int bin_alloc(function *f)
{
  int pagesz = getpagesize();
  int toalloc = f->binarysz;

  if (f->binarysz % pagesz) {
    toalloc = f->binarysz + (pagesz - f->binarysz%pagesz);
  }
  f->binary = mmap(0, toalloc, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);

  if (f->binary == MAP_FAILED) {
    f->binary = NULL;
    return 1;
  }
  return 0;
}

#endif
