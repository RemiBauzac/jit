#ifndef __JIT_H__
#define __JIT_H__

#include "../lang.h"
#define MAX_OPSZ 32

static inline uint8_t *append(uint8_t *ptr, uint64_t bytes, unsigned int len)
{
  if (ptr) {
        if (len == 1) {
            *ptr = bytes;
    }
        else if (len == 2) {
            *(uint16_t *)ptr = bytes;
    }
        else if (len == 4) {
            *(uint32_t *)ptr = bytes;
        }
    else {
      *(uint64_t *)ptr = bytes;
    }
  }
  return ((uint8_t *)ptr) + len;
}

static inline uint8_t *appendoff(uint8_t *ptr, uint32_t offset) {
    *(uint32_t *)ptr = offset;
    return ptr + sizeof(uint32_t);
}

#define APPEND(bytes, len)        do { prog = append(prog, bytes, len); } while (0)
#define APPENDOFF(offset)         do { prog = appendoff(prog, offset); } while (0)

#define APPEND1(b1)               APPEND(b1, 1)
#define APPEND2(b1, b2)           APPEND((b1) + ((b2) << 8), 2)
#define APPEND3(b1, b2, b3)       APPEND2(b1,b2);APPEND1(b3)
#define APPEND4(b1, b2, b3, b4)   APPEND((b1) + ((b2) << 8) + ((b3) << 16) + ((b4) << 24), 4)


#ifdef __x86_64__
#include "arch/x86_64.h"
#else
#error Unsupported architecture
#endif

#if defined(__linux__) || defined(__APPLE__)
#include "os/unix.h"
#endif


static uint8_t *create_op_return(uint8_t *bin, operation *op)
{
	uint8_t *prog = bin;
	/* mov op->param, %rax */
	APPEND2(0x48, 0xb8);
	APPEND(op->param, 8);
	return prog;
}

/* Table of function to create binary from operation */
static uint8_t *(*create_op[NUM_OPCODES]) (uint8_t *bin, operation *op) =
{
	NULL, /* OP_NONE */
	create_op_return, /* OP_RETURN */
};

int create_binary(function *f);

#endif
