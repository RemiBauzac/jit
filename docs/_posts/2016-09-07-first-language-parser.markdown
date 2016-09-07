---
layout: post
title:  "First language parser"
date:   2016-09-07 23:33:20 +0200
author: Rémi Bauzac

---

An interpreted language is divided into 2 parts :

* High level language : human readable (and understanable) source code
* Intermediate language : commonly a binary representation of the high level language transformation. Most of the time, this language is hidden to the user. The intermediate language is interpreted by the virtual machine.

# Definitions

## High level language
The high level language is understanable by a human, so that he can easily interact with it.
As an example, the folowing Lua piece of code :

```
local a = 4
local b = 5
if a > b then print('a is greater than b !') end
```

First, you'll create the simplest language possible in order to focus on the structure of the components. This language only accepts the keyword _return_, followed by an 64 bit integer (positive or negative) : hard to make simpler :

```
return 3
```

## Intermediate language
The intermediate language is mainly a command list (called _operations_) which represent high level language as a binary structure. Each operation contains an action (called _operation code_) and parameters. The intermediate language is commonly called _bytecode_.
In this example, you just need one _operation code_ : `OP_RETURN`
You can represent list of _operation code_ like the following C enum :

```
typedef enum {
  OP_RETURN,
} opCode;
```
You need to ceate the _operation_ structure with _operation code_ and parameters. In this example, you just need a 64 bits integer as the return value of `OP_RETURN` _operation code_.

```
typedef struct _operation {
  opCode op;
  int64_t param;
} operation;
```

# Implementation
You need to create three main components :

* Compiler to transform high level language to intermediate language with :
  * Lexical and syntax analysis
  * Intermediate language generation
  * Optimisations (optional)
* Interpreter to execute commands
* JiT compiler to create executable code

This blog post is focus on compiler.

The compiler is build with :

* [flex](http://flex.sourceforge.net) for the lexical analysis:
  * Lexical definitions as Input
  * Build a minimal automate to reconnaitre the lexical definition
* [bison](http://savannah.gnu.org/git/?group=bison) for the syntax analysis:
  * Traduction schema as input (grammar and actions) 
  * Build a syntax analyser for the schema
  
This blog is not the place to describe **flex** and **bison** behavior. Internet is a gold mine to find documents and tutorials.

## syntax analysis
For syntax analysis, you need to use `yacc` (Yet Another Compiler Compiler), a **bison** implementation. [This article](http://www.linux-france.org/article/devl/lexyacc/minimanlexyacc-3.html) explain how `yacc` work.

A `yacc` file is divided into 3 parts:

* Declarations :
  * target language declaration (in C)
  * tokens (%token)
  * the current token (%union)
  * grammar axiom (%start) 
* grammar productions
* additionnal code, contains `main()`  function declatation and `yyerror()` function (called on syntax error)

The `yacc` declaration part (in C language) looks like :

```
/* native language declarations */
%{
#include "lang.h"

/* parsing variables used by lex and yacc */
int yylex(void);
void yyerror(char *s);
extern FILE *yyin;
extern FILE *yyout;

/* lang binary output creation */
operation op;
function fmain;
FILE *out;
static void append_code(function *f, operation o);
%}
```

The grammar production part for this example is :

```
%%

program:
  main
  ;

main:
  |
  RETURN INTEGER { op.op = OP_RETURN ; op.param = $2; append_code(&fmain, op); }
  ;

%%
```

And finally additionnal code for :

* Error handling

```
/* Parsing error handler */
void yyerror(char *s) {
  extern int yylineno;  // defined and maintained in lexer
  extern char *yytext;  // defined and maintained in lexer
  fprintf(stderr, "%d: %s at %s\n", yylineno, s, yytext);
}
``` 
* A function to append code in code list (intermediate language)

```
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
```

* a function to dump the intermediate language in a file

```
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
```

* and finally `main()` function:

```
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
  fmain.codesz = 0;
  fmain.code = NULL;

  yyparse();
  /* if parsing is ok, dump main function to file */
  dump_function(out, &fmain);
  fclose(yyin);
  fclose(out);
}

```

## Lexical analysis
The lexical analysis is divided into three parts:

* Declarations between `\{\%` and `\%\}`, inserted at the beginning of the syntax analysis code.
* Productions, between `%%`, like `regular expression  action;`
* Additionnal code at the end. The `yywrap` function is called at the end of parsing. 

In the example, the declarations are describe in parser.l :

```
%{
#include "sparser.yacc.h"
void yyerror(char *);
%}
```

Production between `%%` in the same file :

```
"return" return RETURN;
[ \t\n]+ ; /* ignoring whitespaces */
[0-9]+ {
  yylval.integer = strtoll(yytext, (char **)NULL, 10);
  return INTEGER;
}
-[0-9]+ {
  yylval.integer = strtoll(yytext, (char **)NULL, 10);
  return INTEGER;
}
.  yyerror("Unknown character");
```

And finally the additionnal code, shorter as possible :

```
int yywrap(void) { return 1; }
```

# Build
The code is [here](https://github.com/RemiBauzac/jit/tree/first-language-parser). To build the parser, clone project and run make. The code build easily on Linux or MacOS

```
$ git clone git@github.com:RemiBauzac/jit.git
$ cd jit && git checkout first-language-parser
$ cd src && make
$ ...
```

# Run
To run parser, you have just to run langc binary without parameter: `./langc`. It now wait for commands. You have just to type `return 69` then press enter and Ctrl-D. The parser exits and create out.bc file.
The out.bc file is the intermediate language (.bc as bytecode).

To check the result : run `hexdump out.bc`. Please find the result analysis :

```
$ hexdump out.bc
0000000 ef be ad de 01 00 00 00 01 00 00 00 01 00 00 00
0000010 45 00 00 00 00 00 00 00
0000018
```
* `ef be ad de` is the cookie to know that the bytecode is yours
* the first `01 00 00 00` is the version (please note the big endian representation of integers)
* the second `01 00 00 00` is the code size
* the third `01 00 00 00` is the opcode (OP_RETURN)
* `45 00 00 00 00 00 00 00` is 64 bits big endian representation of 69

# What's next
The next post will be about loading and interpreting this bytecode. See you soon.

