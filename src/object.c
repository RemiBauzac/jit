#include <stdio.h>
#include <stdlib.h>

#include "htable.h"
#include "object.h"

/* function functions */
object *newfunction(const char *name)
{
	object *func = malloc(sizeof(object));
	object *code, *vars, *consts;

	htable *ht = NULL;
	if (!func) return NULL;

	func->type = OBJ_FUNC;
  objf(func) = ht_create(32);
	if (!objf(func)) {
		freeobj(func);
		return NULL;
	}
	ht = objf(func);
	/* set function informations */
	ht_set(ht, "name", newbin((uint8_t *)name, strlen(name)+1));

	/* Init void code for function */
	code = newbin(NULL, 0);
	if (!code) {
		freeobj(func);
		return NULL;
	}
  ht_set(ht, FUNC_CODE, code);

	/* Init hash table for vars */
	vars = newtable(32);
	if (!vars) {
		freeobj(func);
		return NULL;
	}
	ht_set(ht, FUNC_VARS, vars);

	/* Init consts */
	consts = newtable(32);
	if (!consts) {
		freeobj(func);
		return NULL;
	}

  return func;
}

/* table functions */
object *newtable(size_t size)
{
	object *table = malloc(sizeof(object));
	if (!table) return NULL;

	table->type = OBJ_TABLE;
	setobjt(table, ht_create(size));
	if (!objt(table)) return NULL;
	return table;
}

/* binary string function */
object *newbin(uint8_t *data, size_t datalen)
{
	object *obj = malloc(sizeof(struct _obj));
	if (!obj) return NULL;
	if (datalen > 0 && data) {
		objbd(obj) = malloc(datalen);
		memcpy(objbd(obj), data, datalen);
		objbsz(obj) = datalen;
	}
	else {
		objbd(obj) = NULL;
		objbsz(obj) = 0;
	}
	return obj;
}

object *appendbin(object *obj, uint8_t *data, size_t datalen)
{
	uint8_t *new = NULL;
	if (!obj) return NULL;
	new = realloc(objbd(obj), objbsz(obj) + datalen);
	if (!new) {
		/* Do not remove free old pointer */
		return NULL;
	}
	objbd(obj) = new;
	memcpy(objbd(obj) + objbsz(obj), data, datalen);
  objbsz(obj) += datalen;
  return obj;
}

void freeobj(object *obj)
{
	if (!obj) return;

	if (objisb(obj) && objbd(obj) != NULL) {
		free(objbd(obj));
	}
	else if (objist(obj) || objisf(obj)) {
		ht_free(objt(obj));
	}
	free(obj);
}
