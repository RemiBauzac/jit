#ifndef __X86_64_COMMON_H__
#define __X86_64_COMMON_H__

#include "../jit.h"

#define NOP  APPEND1(0x90);

#define JIT_PROLOGUE \
	APPEND4(0x55, 0x48, 0x89, 0xe5); /* push %rbp; mov %rsp,%rbp */ \

#define JIT_EPILOGUE \
	APPEND2(0xc9, 0xc3); /* leaveq; retq */

#endif
