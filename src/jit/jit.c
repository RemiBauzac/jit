#include "jit.h"

int create_binary(function *f) {

	uint8_t *prog, *orig;
	int pc, r = -1;

	/* sanity check */
	if (!f) return r;

	/**
	 * Alloc temporary buffer.
	 * Its goal is:
	 *   - build the binary
	 *   - know the binary size to allocate executable memory
	 */
	prog = malloc(f->codesz*MAX_OPSZ);
	if (!prog) return r;

	/**
	 * Start to write binary data into temporary buffer.
	 * Begin with function prologue,
	 * Then write data for each operations
	 * End with function epilogue
	 */
	orig = prog;
	/* Write function prologue to prog */
	JIT_PROLOGUE;
	for (pc = 0; pc < f->codesz; pc++) {
		operation op = f->code[pc];
		if (create_op[op.op]) prog = create_op[op.op](prog, &op);
	}
	JIT_EPILOGUE;

	/* Get size and alloc memory */
	f->binarysz = prog - orig;
	if ((r = bin_alloc(f)) != 0) {
		f->binarysz = 0; f->binary = NULL;
		return r;
	}

	/* copy data to executable memory */
	memcpy(f->binary, orig, f->binarysz);

	return 0;
}
