#ifndef __OBJECT__H__
#define __OBJECT__H__

#define OBJ_TAG    0xff

#define OBJ_NONE   0x00
#define OBJ_INT    0x01
#define OBJ_DEC    0x02
#define OBJ_BIN    0x03
#define OBJ_TABLE  0x04
#define OBJ_FUNC   0x05
#define OBJ_NULL   0x06

struct _htable;

struct _bin {
	size_t   size;
	uint8_t  *data;
};

typedef struct _obj {
	uint8_t type;
	union {
		uint64_t				integer;
		double					decimal;
		struct _bin			bin;
		struct _htable	*t;
	}_v;
} object;

#define objisi(o) (o->type == OBJ_INT)
#define objisd(o) (o->type == OBJ_DEC)
#define objisb(o) (o->type == OBJ_BIN)
#define objist(o) (o->type == OBJ_TABLE)
#define objisf(o) (o->type == OBJ_FUNC)
#define objisn(o) (o->type == OBJ_NULL)

#define obji(o)   ((o)->_v.integer)
#define objd(o)   ((o)->_v.decimal)
#define objb(o)   ((o)->_v.bin)
#define objbsz(o) ((o)->_v.bin.size)
#define objbd(o)  ((o)->_v.bin.data)
#define objf(o)   ((o)->_v.t)
#define objt(o)   ((o)->_v.t)

#define setobji(o, i)    {(o)->type = OBJ_INT;(o)->_v.integer = (i);}
#define setobjd(o, d)    {(o)->type = OBJ_DEC;(o)->_v.decimal = (d);}
#define setobjb(o, d, s) {(o)->type = OBJ_BIN;(o)->_v.bin.data = (d);(o)->_v.bin.size = (s);}
#define setobjf(o, f)    {(o)->type = OBJ_FUNC;(o)->_v.t = (f);}
#define setobjt(o, tbl)    {(o)->type = OBJ_TABLE;(o)->_v.t = (tbl);}

#define FUNC_CODE  "code"
#define FUNC_CONST "consts"
#define FUNC_VARS   "vars"
#define FUNC_ARGS  "args"
#define FUNC_FUNCS "functions"

#ifdef JIT
#define FUNC_JIT "jit"
#endif

object *newfunction(const char *name);
object *newtable(size_t size);
object *newbin(uint8_t *data, size_t datalen);
object *appendbin(object *obj, uint8_t *data, size_t datalen);
void freeobj(object *obj);



#endif
