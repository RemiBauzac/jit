/* native language declarations */
%{
#include "lang.h"
#include "htable.h"

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
static uint32_t add_local(function *f, char *var);
static uint32_t lookup_local(function *f, char *var);
static uint32_t add_k(function *f, value v);

static void dump_function(FILE *out, function *f);
%}


/* data structure for tokens */
%union {
  int64_t integer;
  double  flt;
  char *s;
  uint32_t sp;
  uint32_t op;
};

/* tokens definition. INTEGER and RETURN are defined in lexer (parser.l) */
%token <integer> INTEGER
%token <flt> FLOAT
%token RETURN
%token LOAD 
%token LOCAL
%token ENDCMD
%token <op>OPERATOR 
%token <s> VAR

%type <sp>value

/* start grammar axiom */
%start program

%%

program:
  program statement
  |
  ;

statement:
  RETURN value ENDCMD {
    op.op = OP_RETURN ;
    op.r = $2;
    append_code(fmain, op);
  }
  | VAR LOAD expression ENDCMD {
    op.r = add_local(fmain, $1);
    append_code(fmain, op);
  }
  | VAR LOAD value ENDCMD {
    op.r = add_local(fmain, $1);
    op.a = $3; op.op = OP_LOAD;
    append_code(fmain, op);
  }
  ;

value:
  INTEGER {
    setivalue(v, $1);
    op.a = add_k(fmain, v);
    op.r = fmain->stacksz++;
    op.op = OP_LOAD_CONST;
    append_code(fmain, op);
    $$ = op.r;
  }
  | VAR {
    $$ = lookup_local(fmain, $1);
  }
  ;

expression:
  | value OPERATOR value {
    op.a = $1; op.b = $3;
    op.op = $2;
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
  /* clear operator */
  memset(&o, 0, sizeof(o));
}

/**
 * append a value to const value list
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

/**
 * add a local var to var list
 */
static uint32_t add_local(function *f, char *var)
{
  value v, *r;

  r = ht_get(f->vars, var);
  if (!r) {
    setivalue(v, f->stacksz);
    ht_set(f->vars, var, v);
    f->stacksz += 1;
    r = ht_get(f->vars, var);
  }
  return ivalue(*r);
}

static uint32_t lookup_local(function *f, char *var)
{
  value *r;

  r = ht_get(f->vars, var);
  if (!r) {
    yyerror("cannot find variable");
    return 0;
  }
  return ivalue(*r);
}

static void dump_function(FILE *out, function *f)
{
  uint32_t cookie = LANG_COOKIE, version = LANG_VERSION;
  if (!out || !f) return;
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

  /* Init variables */
  fmain = malloc(sizeof(function));
  if (!fmain) {
    fprintf(stderr, "Cannot alloc main function\n");
    exit(ENOMEM);
  }
  memset(&op, 0, sizeof(operation));

  /**
   * init fmain
   */
   memset(fmain, 0, sizeof(function));
   fmain->vars = ht_create(100);

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

  if (yyparse() == 0) {
    /* if parsing is ok, dump main function to file */
    /* open output file */
    out = fopen(outname, "wb");
    if (!out) {
      fclose(yyin);
      fprintf(stderr, "Cannot open output file %s\n", outname);
      exit(ENOENT);
    }
    dump_function(out, fmain);
  }
  free_function(fmain);
  fclose(yyin);
  fclose(out);
}
