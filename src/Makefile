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
COMMON_O=htable.o object.o marshal.o
VM_O=vm.o
JIT_O=jit/jit.o
LANGC=langc
LANGVM=langvm

all:$(LANGC) $(LANGVM)
CFLAGS=-g

o: $(PARSER_O) $(VM_O) $(COMMON_O)

$(LEXC) $(LEXH):
	$(LEX) --outfile=$(LEXC) --header-file=$(LEXH) $(LEXL)

$(YYC) $(YYH):
	$(YACC) --defines=$(YYH) --output-file=$(YYC) $(YYY)

sparser.lex.o: $(LEXC) $(LEXH)
sparser.yacc.o: $(YYC) $(YYH)

$(LANGC): $(PARSER_O) $(COMMON_O)
	$(CC) -o $@ $+

$(LANGVM): $(VM_O) $(JIT_O) $(COMMON_O)
	$(CC) -o $@ $+

clean:
	$(RM) -fr $(LEXC) $(LEXH) $(YYC) $(YYH) $(PARSER_O) $(COMMON_O) $(LANGC) $(LANGVM) $(VM_O) $(JIT_O)
