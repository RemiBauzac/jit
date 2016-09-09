#include "jit.h"

int create_binary(function *f) {

	uint8_t *prog, *tmp, *orig;

	/* sanity check */
	if (!f) return -1;

	/**
	 * Alloc temporary buffer.
	 * Its goal is:
	 *   - build the binary
	 *   - know the binary size to allocate executable memory
	 */
	prog = malloc(f->codesz*MAX_OPSZ);
	if (!prog) return -1;

	return 0;
}
