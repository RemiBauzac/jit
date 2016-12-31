#ifndef __LANG_VALUE__
#define __LANG_VALUE__

/**
 * Values
 */

/* values types */
#define TYPE_NONE 0
#define TYPE_INT  1
#define TYPE_FLT  2

typedef struct _value {
  uint32_t type;
  union {
    void *ptr;
    int64_t integer;
    double   flt;
  }vu;
} value;

#define setivalue(v, i) (v).vu.integer = i; v.type = TYPE_INT;
#define setfvalue(v, f) (v).vu.flt = f;  v.type = TYPE_FLT;

#define ivalue(v) (v).vu.integer
#define fvalue(v) (v).vu.flt

#define is_n(v) ((v).type == TYPE_NONE)
#define is_i(v) ((v).type == TYPE_INT)
#define is_f(v) ((v).type == TYPE_FLT)

static value none = {
  .type = TYPE_NONE,
  .vu.ptr = NULL
};


#endif
