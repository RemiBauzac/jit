#ifndef __HTABLE_H__
#define __HTABLE_H__

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "lang_value.h"

struct _value;

typedef struct _hentry {
  char *key;
  struct _value v;
  struct _hentry *next;
  struct _hentry *prev;
} hentry;

typedef struct _htable {
  size_t size;
  size_t entrysz;
  hentry **table;
} htable;

static inline size_t hash(htable *ht, char *k, size_t kl)
{
	size_t i, h;
	for (h = i = 0; i < kl; h = h << 8, h += k[ i++ ]);
	return h % ht->size;
}

static inline void free_entry(hentry *e)
{
  if (!e) return;
  if (e->key) free(e->key);
  free(e);
}

htable *ht_create(size_t size);
void ht_set( htable *ht, char *key, value v);
value *ht_get(htable *ht, char *key);
void ht_release(htable *ht, char *key);

#endif
