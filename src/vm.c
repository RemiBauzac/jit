#include "lang.h"
#include "jit/jit.h"

static inline void free_function(function *f)
{
	free(f->code);
	bin_free(f);
	free(f);
}

static function *load_main(const char *filename)
{
	function *main = NULL;
	FILE *bcf;
	uint32_t cookie = 0, version = 0;
	size_t codesz;

	/* Open file */
	bcf = fopen(filename, "rb");
	if (!bcf) {
		fprintf(stderr, "Cannot open %s bytecode\n", filename);
		return NULL;
	}

	/* Check cookie */
	fread(&cookie, sizeof(uint32_t), 1, bcf);
	cookie = ntohl(cookie);
	if (cookie != LANG_COOKIE) {
		fprintf(stderr, "Bad bytecode file format\n");
		fclose(bcf);
		return NULL;
	}

	/**
	 * Check version.
	 * Useless now, but it will be helpful later
	 */
	fread(&version, sizeof(uint32_t), 1, bcf);
	version = ntohl(version);
	if (version != LANG_VERSION) {
		fprintf(stderr, "Bad lang version %d. %d was awaited\n", version, LANG_VERSION);
		return NULL;
	}

	/* Alloc main function structure */
	main = malloc(sizeof(function));
	if (!main) return NULL;
	memset(main, 0, sizeof(function));

	/* Read 32 bits code size from file */
	fread(&main->codesz, sizeof(uint32_t), 1, bcf);
	main->codesz = ntohl(main->codesz);
	if (main->codesz == 0) {
		fprintf(stderr, "Empty bytecode file\n");
		return main;
	}

	/* Alloc code regarding code size */
	main->code = malloc(sizeof(operation)*main->codesz);

	/* Put code to structure */
	codesz = fread(main->code, sizeof(operation), main->codesz, bcf);
	if (codesz != main->codesz) {
		fprintf(stderr, "Error reading bytecode in file\n");
		free(main->code);
		free(main);
		return NULL;
	}
	return main;
}


static int64_t interpret(function *fmain)
{
	int pc;
	int64_t ret = 0;
	operation op;

	/* walking through main function */
	for(pc = 0; pc < fmain->codesz; pc++) {
		op = fmain->code[pc];
		switch(op.op) {
			/* On return, get the return value from parameters and exit */
			case OP_RETURN:
				ret = op.a;
				break;
			/* Handle error case */
			case OP_NONE:
			default:
				fprintf(stderr, "Unavailable operand %u\n", op.op);
				exit(1);
		}
	}
	return ret;
}

int main(int argc, char **argv)
{
	function *fmain;
	int64_t ret = 0;
	int jit = 0;
	char *filename;

	if (argc == 3 && !strcmp(argv[1], "-j")) {
		jit = 1;
		filename = argv[2];
	}
	else if (argc == 2) {
		filename = argv[1];
	}
	else {
		fprintf(stderr, "usage: %s [-j] <file>\n", argv[0]);
		exit(1);
	}
	/* load main function, exit on error */
	fmain = load_main(filename);
	if (!fmain) exit(1);

	/* create executable */
	if (jit) {
		create_binary(fmain);
	}

	if (fmain->binary) {
		/* cast binary into function pointer */
		uint64_t (*execute)(void) = (void *)fmain->binary;
		/* and call this function */
		ret = execute();
	}
	else {
		/* start interpreter, get return value and print it */
		ret = interpret(fmain);
	}
	fprintf(stdout, "Return value is %lld\n", ret);

	/* cleanly exit from interpreter */
	free_function(fmain);
	exit(0);
}
