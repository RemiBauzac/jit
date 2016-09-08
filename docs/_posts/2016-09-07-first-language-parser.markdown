---
layout: post
title:  "First language parser"
date:   2016-09-07 23:33:20 +0200
author: Rémi Bauzac

---

An interpreted language is divided in 2 parts:

* High level language: human readable (and understandable) source code
* Intermediate language: commonly, a binary representation of the high level language transformation. Most of the time, this language is hidden to the user. The intermediate language is interpreted by the virtual machine.

# Definitions

## High level language
The high level language is understandable by a human, so that he can easily interact with it.
As an example, the following Lua piece of code:

{% highlight lua %}
local a = 4
local b = 5
if a > b then print('a is greater than b !') end
{% endhighlight %}

First, you'll create the simplest language possible in order to focus on the structure of the components. This language only accepts the keyword `return`, followed by a 64 bits integer (positive or negative): hard to make simpler:

```
return 3
```

## Intermediate language
The intermediate language is mainly a command list (called _operations_) which represents high level language as a binary structure. Each operation contains an action (called _operation code_) and parameters. The intermediate language is commonly called _bytecode_.
In this example, you just need one _operation code_ : `OP_RETURN`.

You can represent the list of possible _operation codes_ like the following C enum:

{% highlight c %}
typedef enum {
  OP_RETURN,
} opCode;
{% endhighlight %}

You need to ceate the _operation_ structure with _operation code_ and parameters. In this example, you just need a 64 bits integer as the return value of `OP_RETURN` _operation code_.

{% highlight c %}
typedef struct _operation {
  opCode op;
  int64_t param;
} operation;
{% endhighlight %}

# Implementation
You need to create three main components:

* The compiler, to transform the high level language to an intermediate language, with:
  * Lexical and syntax analysis
  * Intermediate language generation
  * Optimisations (optional)
* The interpreter, to execute commands
* And the JiT compiler, to create executable code.

This blog post is focused on the compiler.

The compiler is build with:

* [flex](http://flex.sourceforge.net) for the lexical analysis:
  * Lexical definitions as input
  * Build a minimal automate to identify the lexical definition
* [bison](http://savannah.gnu.org/git/?group=bison) for the syntax analysis:
  * Translation schema as input (grammar and actions) 
  * Build a syntax analyser for the schema
  
This blog is not the place to describe **flex** and **bison** behavior. Internet is a gold mine to find documents and tutorials.

## syntax analysis
For syntax analysis, you need to use `yacc` (Yet Another Compiler Compiler), a **bison** implementation. [This article](http://www.linux-france.org/article/devl/lexyacc/minimanlexyacc-3.html) explains how `yacc` work.

A `yacc` file is divided in 3 parts:

* Declarations:
  * target language declaration (in C)
  * tokens (`%token`)
  * the current token (`%union`)
  * grammar axiom (`%start`) 
* grammar productions
* additionnal code, which contains the `main()` function declaration and the `yyerror()` function (called on syntax error)

The `yacc` declaration part (in C language) looks like:

{% highlight c %}
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
{% endhighlight %}

The grammar production part for this example is:

{% highlight c %}
%%

program:
  main
  ;

main:
  |
  RETURN INTEGER { op.op = OP_RETURN ; op.param = $2; append_code(&fmain, op); }
  ;

%%
{% endhighlight %}

And, finally, additionnal code for:

* Error handling

{% highlight c %}
/* Parsing error handler */
void yyerror(char *s) {
  extern int yylineno;  // defined and maintained in lexer
  extern char *yytext;  // defined and maintained in lexer
  fprintf(stderr, "%d: %s at %s\n", yylineno, s, yytext);
}
{% endhighlight %}

* A function to append code in code list (intermediate language)

{% highlight c %}
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
{% endhighlight %}

* a function to dump the intermediate language to a file

{% highlight c %}
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
{% endhighlight %}

* and, finally, the `main()` function:

{% highlight c %}
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
{% endhighlight %}

## Lexical analysis
The lexical analysis is divided in three parts:

* Declarations, between `\{\%` and `\%\}`, inserted at the beginning of the syntax analysis code.
* Productions, between `%%`, like `regular expression  action;`
* Additionnal code at the end. The `yywrap` function is called at the end of parsing. 

In the example, the declarations are describe in `parser.l`:

{% highlight c %}
%{
#include "sparser.yacc.h"
void yyerror(char *);
%}
{% endhighlight %}

Production between `%%` in the same file :

{% highlight c %}
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
{% endhighlight %}

And finally, the additionnal code, as short as possible:

{% highlight c %}
int yywrap(void) { return 1; }
{% endhighlight %}

# Build
The code is [here](https://github.com/RemiBauzac/jit/tree/first-language-parser). To build the parser, clone the project and run `make`. The codes build easily on Linux or MacOS:

{% highlight shell %}
$ git clone git@github.com:RemiBauzac/jit.git
$ cd jit && git checkout first-language-parser
$ cd src && make
$ ...
{% endhighlight %}

# Run
To run the parser, you have just to run the `langc` binary, without any parameter: `./langc`. It now waits for commands. You just have to type `return 69` then press <kbd>enter</kbd> and <kbd>Ctrl-D</kbd>. The parser exits and creates the `out.bc` file.  
This `out.bc` file is the intermediate language (`.bc` as bytecode).

To check the result, run `hexdump out.bc`. Here is the resulting analysis:

{% highlight shell %}
$ hexdump out.bc
0000000 ef be ad de 01 00 00 00 01 00 00 00 01 00 00 00
0000010 45 00 00 00 00 00 00 00
0000018
{% endhighlight %}

* `ef be ad de` is the cookie to know that the bytecode is yours
* the first `01 00 00 00` is the version (please note the big endian representation of integers)
* the second `01 00 00 00` is the code size
* the third `01 00 00 00` is the opcode (OP_RETURN)
* `45 00 00 00 00 00 00 00` is 64 bits big endian representation of `69`

# What's next
The next post will be about loading and interpreting this bytecode. See you soon.

