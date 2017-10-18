#include <stdio.h>
#include <stdlib.h>

#include "htable.h"
#include "object.h"

/* Dump functions */
static size_t integer2file(FILE *dest, object *o)
{
	size_t size = sizeof(uint8_t) + sizeof(uint64_t);
	if (!o) return 0;
	if (dest) {
		fwrite(&o->type, sizeof(uint8_t), 1, dest);
		fwrite(&obji(o), sizeof(uint64_t), 1, dest);
	}
	return size;
}

static size_t decimal2file(FILE *dest, object *o)
{
	size_t size = sizeof(uint8_t) + sizeof(double);
	if (!o) return 0;
	if (dest) {
		fwrite(&o->type, sizeof(uint8_t), 1, dest);
		fwrite(&objd(o), sizeof(double), 1, dest);
	}
	return size;
}

static size_t bin2file(FILE *dest, object *o)
{
	size_t size;

	if (!o) return 0;

	size += sizeof(uint8_t) + sizeof(uint64_t) + objbsz(o);

	if (dest) {
		fwrite(&o->type, sizeof(uint8_t), 1, dest);
		fwrite(&objbsz(o), sizeof(uint64_t), 1, dest);
		fwrite(objbd(o), 1, objbsz(o), dest);
	}
	return size;
}

static size_t htable2file(FILE *dest, object *o)
{
	char **keys = NULL;
	char *key;
	int i;
	size_t keysize, size = sizeof(uint8_t) + sizeof(size_t);
	htable *ht = objt(o);

	if (!objist(o)) {
		return 0;
	}

	if (dest) {
		fwrite(&o->type, sizeof(uint8_t), 1, dest);
		fwrite(&ht->entrysz, sizeof(size_t), 1, dest);
	}

	keys = ht_keys(ht);
  for (i = 0; keys[i] != NULL; i++) {
		object *obj = ht_get(ht, key);
		if (!obj) continue;

		/* put key on file */
		keysize = strlen(key)+1;
		if (dest) {
			fwrite(&keysize, sizeof(size_t), 1, dest);
			fwrite(key, 1, keysize, dest);
		}
		size += sizeof(size_t) + keysize;

		switch (obj->type) {
			case OBJ_INT:
				size += integer2file(dest, obj);
				break;
			case OBJ_DEC:
				size += decimal2file(dest, obj);
				break;
			case OBJ_BIN:
				size += bin2file(dest, obj);
				break;
			case OBJ_TABLE:
			case OBJ_FUNC:
				size += htable2file(dest, obj);
				break;
			defautl:
				break;
		}
	}
	return size;
}
