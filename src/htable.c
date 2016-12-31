#include "htable.h"

htable *ht_create(size_t size)
{

	htable *ht = NULL;
	size_t i;

	if (size < 1) return NULL;

	/* Allocate the table itself. */
	ht = malloc(sizeof(htable));
	if (!ht) {
		fprintf(stderr, "Cannot alloc htable\n");
		return NULL;
	}
	ht->table = malloc(sizeof(hentry *)*size);
	if (!ht->table) {
		free(ht);
		fprintf(stderr, "Cannot alloc htable\n");
		return NULL;
	}
	memset(ht->table, 0, sizeof(hentry *)*size);
	ht->size = size;

	return ht;
}

static hentry *ht_newentry(char *key, value v) {
	hentry *ne;
	size_t keylen;

	if (!key) return NULL;
	keylen = strlen(key)+1;

  ne = malloc(sizeof(hentry));
  if (!ne) {
     fprintf(stderr, "Cannot alloc htable pair\n");
     return NULL;
  }

  ne->key = malloc(keylen);
  if (!ne->key) {
    fprintf(stderr, "Cannot alloc htable key pair\n");
    free(ne);
    return NULL;
  }
	snprintf(ne->key, keylen, "%s", key);
  ne->v = v;
	return ne;
}

void ht_set(htable *ht, char *key, value v) {
	size_t head = 0, keylen;
	hentry *ne = NULL;
	hentry *crt = NULL;

  if (!key) return;

  keylen = strlen(key)+1;
	head = hash(ht, key, keylen);
  crt = ht->table[head];

  /* try to find entry */
  while(crt && strcmp(key, crt->key)) {
    crt = crt->next;
  }
  if (crt) {
    /* yes, find it, have to replace value */
    crt->v = v;
  }
  else {
    /* create new entry */
    ne = ht_newentry(key, v);
    if (!ne) return;

    ht->entrysz++;
    /* i'm the first entry of the set */
    if (ht->table[head] == NULL) {
      ht->table[head] = ne;
      ne->next = NULL;
      ne->prev = ne;
    }
    else {
	    hentry *last = ht->table[head]->prev;

      /* insert new entry at the end */
      last->next = ne;
      ne->prev = last;
      ne->next = NULL;
      ht->table[head]->prev = ne;
    }
  }
}

static hentry *_ht_get(htable *ht, char *key, size_t keylen) {
	size_t head  = 0;
	hentry *e;

	head = hash(ht, key, keylen);

	/* Step through the bin, looking for our value. */
	e = ht->table[head];
	while (e && strcmp(key, e->key)) {
		e = e->next;
	}
  return e;
}

value *ht_get(htable *ht, char *key) {
  if (!key) return NULL;

  hentry *p = _ht_get(ht, key, strlen(key)+1);
  return p?&p->v:NULL;
}

void ht_release(htable *ht, char *key)
{
	size_t head=0, keylen;

  if (key) {
    keylen = strlen(key)+1;
	  head = hash(ht, key, keylen);
    hentry *e = _ht_get(ht, key, keylen);

    if (e->next == NULL && e->prev == e) {
      /* I'm alone */
      ht->table[head] = NULL;
    }
    else if (e->next == NULL) {
      /* I'm last element */
      e->prev->next = e->next;
      ht->table[head] = e->prev;
    }
    else {
      e->prev->next = e->next;
      e->next->prev = e->prev;
    }
    free_entry(e);
    ht->entrysz--;
  }
  else {
  }
}
