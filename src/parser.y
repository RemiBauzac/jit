/* native language declarations */
%{
#include "lang.h"

/* parsing variables used by lex and yacc */
int yylex(void);
void yyerror(char *s);
extern FILE *yyin;
extern FILE *yyout;

/* tiny binary output */
operation op;
object *fmain;
htable *code;

FILE *out;
static void append_code(function *f, operation o);
static void dump_function(FILE *out, function *f);
%}


/* data structure for tokens */
%union {
	int64_t integer;
};

/* tokens definition. INTEGER and RETURN are defined in lexer (sparser.l) */
%token <integer> INTEGER
%token RETURN

/* start grammar axiom */
%start program

%%

program:
	main
	;

main:
	|
	RETURN INTEGER {
		op.op = OP_RETURN;
		setobji(&op.param, $2);
		append_code(code, op);
	}
	;

%%

/* Parsing error handler */
void yyerror(char *s) {
	extern int yylineno;  // defined and maintained in lexer
	extern char *yytext;  // defined and maintained in lexer
	fprintf(stderr, "%d: %s at %s\n", yylineno, s, yytext);
}

/**
 * append code to code list for a function.
 * This function is suboptimal but easy to understand.
 * We have to change it to avoid realloc on each operation
 */
static void append_code(htable *code, operation o)
{
  
	f->codesz += 1;
	f->code = realloc(f->code, f->codesz*sizeof(o));
	if (f->code)
		f->code[f->codesz-1] = o;
}

/**
 * dump_function function:
 * used to dump a function to a file
 */
static void dump_function(FILE *out, function *f)
{
  int cookie = LANG_COOKIE, version = LANG_VERSION;
  if (!out || !f) return;
  fwrite(&cookie,  sizeof(char), 4, out);
  fwrite(&version, sizeof(char), 4, out);
  fwrite(&f->codesz, sizeof(uint32_t), 1, out);
  fwrite(f->code, sizeof(operation), f->codesz, out);
}

/**
 * main parser function:
 * argv[1] is the input file name, by default stdin
 * argv[2] is output file name, by default out.bc
 */
int main(int argc, char **argv) {
  char *outname = "out.bc";

  /* Simple args parsing */
  /* Open input file if needed. If not available, use yyparse use stdin */
	if (argc == 3) {
		yyin = fopen(argv[1], "r");
		if (!yyin) {
			fprintf(stderr, "Cannot open input file %s\n", argv[1]);
			exit(1);
		}
		outname = argv[2];
	}
	else if (argc == 2) {
		yyin = fopen(argv[1], "r");
		if (!yyin) {
			fprintf(stderr, "Cannot open input file %s\n", argv[1]);
			exit(1);
		}
	}

  /* open output file */
	out = fopen(outname, "wb");
	if (!out) {
		fclose(yyin);
		fprintf(stderr, "Cannot open output file %s\n", outname);
		exit(1);
	}
	/* initialise main function */
	fmain = newfunction("main");

	yyparse();
	/* if parsing is ok, dump main function to file */
	dump_function(out, &fmain);
	fclose(yyin);
	fclose(out);
}
