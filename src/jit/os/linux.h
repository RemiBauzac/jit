#ifndef __LINUX_H__
#define __LINUX_H__

#include <malloc.h>
#include <sys/mman.h>
#include <sys/user.h>

static inline void bin_free(function *f)
{
  if (!f) return;
  free(f->binary);
  f->binary = NULL;
}

static inline int bin_alloc(function *f)
{
	f->binary = memalign(PAGE_SIZE, f->binarysz);
  if (!f->binarysz) return 1;

  if (mprotect(f->binary, f->binarysz, PROT_EXEC|PROT_READ|PROT_WRITE) == -1) {
    bin_free(f);
    return 1;
  }
  return 0;
}

#endif
