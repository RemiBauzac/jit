#ifndef __OP_H__
#define __OP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "object.h"

#define LANG_COOKIE  0xdeadbeef
#define LANG_VERSION 0x00000001 /* mean 0.1 */

typedef enum {
	OP_NONE = 0,
	OP_RETURN
} opCode;

#define NUM_OPCODES ((int)OP_RETURN + 1)

typedef struct _operation {
	uint32_t op;
	object param;
} operation;

#endif
