%{
#include "parser.yacc.h"
void yyerror(char *);
%}

%%

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

; return END;

.  yyerror("Unknown character");



%%

int yywrap(void) { return 1; }
