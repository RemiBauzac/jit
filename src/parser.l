%{
#include "parser.yacc.h"
#include "lang_function.h"
void yyerror(char *);
%}

%option yylineno

whitespace  [ \t\n]+
digit       [0-9]
integer     [-]?{digit}+
exponent     [eE][+-]?{integer}
float       [-]?{integer}("."{integer})?{exponent}?
var          [a-zA-Z0-9]+
%%

{whitespace} ; /* ignoring whitespaces */

{integer} {
  yylval.integer = strtoll(yytext, (char **)NULL, 10);
  return INTEGER;
}

{float} {
  yylval.flt = atof(yytext);
  return FLOAT;
}

"return" return RETURN;
"=" return LOAD;
"+" {
  yylval.op = OP_ADD;
  return OPERATOR;
}
"-" {
  yylval.op = OP_SUB;
  return OPERATOR;
}
"*" {
  yylval.op = OP_MUL;
  return OPERATOR;
}

";" return ENDCMD;
{var} {
  yylval.s = strdup(yytext);
  return VAR;
}

.  yyerror("Unknown character");

%%

int yywrap(void) { return 1; }
