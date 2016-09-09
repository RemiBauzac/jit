#ifndef __OP_H__
#define __OP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LANG_COOKIE  0xdeadbeef
#define LANG_VERSION 0x00000001 /* mean 0.1 */

typedef enum {
	OP_NONE = 0,
	OP_RETURN
} opCode;

#define NUM_OPCODES ((int)OP_RETURN + 1)

typedef struct __attribute__((__packed__)) _operation {
	uint32_t op;
	int64_t param;
} operation;

typedef struct _function {
	size_t codesz;
	operation *code;
	uint8_t *binary;
	size_t binarysz;
} function;

#endif
