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
	ht->list = NULL;
	ht->table = malloc(sizeof(hentry *)*size);
	if (!ht->table) {
		free(ht);
		fprintf(stderr, "Cannot alloc htable\n");
		return NULL;
	}
	memset(ht->table, 0, sizeof(hentry *)*size);
	ht->size = size;
	ht->entrysz = 0;
	ht->listsz = 0;
	return ht;
}

void ht_free(htable *ht)
{
	size_t i;
  hentry *hcrt, *old;

	if (!ht || ht->entrysz == 0) {
		return;
	}
	hcrt = ht->list;
	while(hcrt != NULL) {
		free(hcrt->key);
		freeobj(hcrt->o);
		old = hcrt;
		hcrt = hcrt->next;
		free(old);
	}
	for (i = 0; i < ht->size; i++) {
		hcrt = ht->table[i];
		while(hcrt != NULL) {
			free(hcrt->key);
			freeobj(hcrt->o);
			old = hcrt;
			hcrt = hcrt->next;
			free(old);
		}
	}
	free(ht);
}

static hentry *ht_newentry(char *key, object *o) {
	hentry *ne;
	size_t keylen;

	if (!o || !key) return NULL;
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
	ne->o = o;
	return ne;
}

static hentry *ht_newlistentry(object *o) {
	hentry *ne;

	if (!o) return NULL;

  ne = malloc(sizeof(hentry));
  if (!ne) {
     fprintf(stderr, "Cannot alloc htable pair\n");
     return NULL;
  }
	ne->o = o;
	return ne;
}

void ht_add_tail(htable *ht, object *o)
{
	hentry *last;
	hentry *ne;
	if (!ht || !o) return;

	ne = ht_newlistentry(o);
	last = ht->list->prev;

	/* insert new entry at the end */
	last->next = ne;
	ne->prev = last;
	ne->next = NULL;
	ht->list->prev = ne;
	ht->listsz++;
}

void ht_set(htable *ht, char *key, object *o) {
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
    /* yes, find it, have to replace object */
		crt->o = o;
  }
  else {
    /* create new entry */
    ne = ht_newentry(key, o);
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

	/* Step through the bin, looking for our object. */
	e = ht->table[head];
	while (e && strcmp(key, e->key)) {
		e = e->next;
	}
  return e;
}

object *ht_get(htable *ht, char *key) {
  if (!key) return NULL;

  hentry *p = _ht_get(ht, key, strlen(key)+1);
  return p?p->o:NULL;
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

char **ht_keys(htable *ht)
{
	size_t i, rcount = 0;
	char **ret;
  hentry *hcrt;

	if (!ht || ht->entrysz == 0) {
		return NULL;
	}

	ret = malloc(ht->entrysz*sizeof(char *));
	if (!ret) return NULL;

	for (i = 0; i < ht->size; i++) {
		hcrt = ht->table[i];
		while(hcrt != NULL) {
			ret[rcount++] = strdup(hcrt->key);
			hcrt = hcrt->next;
		}
	}
	return ret;
}
