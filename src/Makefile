LEX=/usr/bin/lex
YACC=/usr/bin/yacc
CC=gcc

# -- Don't touch above this line
# Parser files
LEXL=parser.l
LEXC=parser.lex.c
LEXH=parser.lex.h
YYY=parser.y
YYC=parser.yacc.c
YYH=parser.yacc.h

PARSER_O=parser.yacc.o parser.lex.o
VM_O=vm.o
LANGC=langc
LANGVM=langvm

all:$(LANGC) $(LANGVM)

o: $(PARSER_O) $(VM_O)

$(LEXC) $(LEXH):
	$(LEX) --outfile=$(LEXC) --header-file=$(LEXH) $(LEXL)

$(YYC) $(YYH):
	$(YACC) --defines=$(YYH) --output-file=$(YYC) $(YYY)

sparser.lex.o: $(LEXC) $(LEXH)
sparser.yacc.o: $(YYC) $(YYH)

$(LANGC): $(PARSER_O) 
	$(CC) -o $@ $+

$(LANGVM): $(VM_O) 
	$(CC) -o $@ $+

clean:
	$(RM) -fr $(LEXC) $(LEXH) $(YYC) $(YYH) $(PARSER_O) $(LANGC) $(LANGVM) $(VM_O)
