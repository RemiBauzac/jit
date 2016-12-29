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
value v;
function *fmain;
FILE *out;
static void append_code(function *f, operation o);
static uint32_t add_local(function *f, const char *var);
static uint32_t lookup_local(function *f, const char *var);
static uint32_t add_k(function *f, value v);

static void dump_function(FILE *out, function *f);
%}


/* data structure for tokens */
%union {
  int64_t integer;
  double  flt;
  const char *s;
};

/* tokens definition. INTEGER and RETURN are defined in lexer (sparser.l) */
%token <integer> INTEGER
%token <flt> FLOAT
%token RETURN
%token ADD SUB MUL DIV
%token LOAD 
%token LOCAL
%token ENDCMD
%token <s> VAR

/* start grammar axiom */
%start program

%%

program:
  program statement
  |
  ;

statement:
  expression { }
  | RETURN expression ENDCMD {
    op.op = OP_RETURN ;
    append_code(fmain, op);
  }
  | VAR LOAD expression ENDCMD {
    op.r = add_local(fmain, $1);
    op.wr = LANG_VAR;
    append_code(fmain, op);
  }
  ;

expression:
  INTEGER {
    v.type = TYPE_INT; ivalue(v) = $1;
    op.a = add_k(fmain, v); op.wa = LANG_CONST;
  }
  | VAR {
    uint32_t a = lookup_local(fmain, $1);
    if (a == UINT32_MAX) {
      yyerror("cannot find variable");
      YYERROR;
    }
    op.a = a; op.wa = LANG_VAR; op.op = OP_LOAD;
  }
  | INTEGER ADD INTEGER {
    v.type = TYPE_INT; ivalue(v) = $1;
    op.a = add_k(fmain, v); op.wa = LANG_CONST;
    v.type = TYPE_INT; ivalue(v) = $3;
    op.b = add_k(fmain, v); op.wb = LANG_CONST;
    op.op = OP_ADD;
  }
  ;

%%

/* Parsing error handler */
void yyerror(char *s) {
  extern int yylineno;  // defined and maintained in lexer
  extern char *yytext;  // defined and maintained in lexer
  fprintf(stderr, "Parse error on line %d: %s (%s) \n", yylineno, s, yytext);
}

/**
 * append code to code list for a function.
 * This function is suboptimal but easy to understand.
 * We have to change it to avoid realloc on each operation
 */
static void append_code(function *f, operation o)
{
  f->codesz += 1;
  f->code = realloc(f->code, f->codesz*sizeof(o));
  if (f->code)
    f->code[f->codesz-1] = o;
}

/**
 * append a value to local variables list
 * This function is suboptimal but easy to understand.
 * We have to change it to avoid realloc on each stack need
 */
static uint32_t add_k(function *f, value v)
{
  f->ksz += 1;
  f->k = realloc(f->k, f->ksz*sizeof(v));
  if (f->k)
    f->k[f->ksz-1] = v;
  return f->ksz-1;
}

static uint32_t add_local(function *f, const char *var)
{
  uint32_t i;

  for (i = 0; i < f->stacksz; i++) {
    if (f->vars[i] && !strcmp(f->vars[i], var)) {
      return i;
    }
  }
  /* nothing found about var add it*/
  f->stacksz += 1;
  f->vars = realloc(f->vars, f->stacksz*sizeof(char *));
  f->vars[f->stacksz-1] = var;
  return f->stacksz-1;
}

static uint32_t lookup_local(function *f, const char *var)
{
  uint32_t i;

  for (i = 0; i < f->stacksz; i++) {
    if (f->vars[i] && !strcmp(f->vars[i], var)) {
      return i;
    }
  }
  return UINT32_MAX;
}

static void dump_function(FILE *out, function *f)
{
  uint32_t cookie = LANG_COOKIE, version = LANG_VERSION;
  if (!out || !f) return;
  f->stack = NULL;
  fwrite(&cookie,  sizeof(char), sizeof(uint32_t), out);
  fwrite(&version, sizeof(char), sizeof(uint32_t), out);
  fwrite(&f->codesz, sizeof(size_t), 1, out);
  fwrite(f->code, sizeof(operation), f->codesz, out);
  fwrite(&f->ksz, sizeof(size_t), 1, out);
  fwrite(f->k, sizeof(value), f->ksz, out);
  fwrite(&f->stacksz, sizeof(size_t), 1, out);
}


/**
 * main parser function:
 * argv[1] is the input file name, by default stdin
 * argv[2] is output file name, by default out.bc
 */
int main(int argc, char **argv) {
  char *outname = "out.bc";

  fmain = malloc(sizeof(function));
  if (!fmain) {
    fprintf(stderr, "Cannot alloc main function\n");
    exit(ENOMEM);
  }

  /* Simple args parsing */
  /* Open input file if needed. If not available, use yyparse use stdin */
  if (argc == 3) {
    yyin = fopen(argv[1], "r");
    if (!yyin) {
      fprintf(stderr, "Cannot open input file %s\n", argv[1]);
      exit(ENOENT);
    }
    outname = argv[2];
  }
  else if (argc == 2) {
    yyin = fopen(argv[1], "r");
    if (!yyin) {
      fprintf(stderr, "Cannot open input file %s\n", argv[1]);
      exit(ENOENT);
    }
  }

  /* open output file */
  out = fopen(outname, "wb");
  if (!out) {
    fclose(yyin);
    fprintf(stderr, "Cannot open output file %s\n", outname);
    exit(ENOENT);
  }
  /* initialise main function */
  memset(fmain, 0, sizeof(function));

  yyparse();
  /* if parsing is ok, dump main function to file */
  dump_function(out, fmain);
  free_function(fmain);
  fclose(yyin);
  fclose(out);
}
