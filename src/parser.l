%{
#include "parser.yacc.h"
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

"return" return RETURN;
{whitespace} ; /* ignoring whitespaces */

{integer} {
  yylval.integer = strtoll(yytext, (char **)NULL, 10);
  return INTEGER;
}

{float} {
  yylval.flt = atof(yytext);
  return FLOAT;
}

"+" return ADD;
"-" return SUB;
"*" return MUL;
"/" return DIV;
"=" return LOAD;
";" return ENDCMD;
{var} {
  yylval.s = strdup(yytext);
  return VAR;
}

.  yyerror("Unknown character");

%%

int yywrap(void) { return 1; }
