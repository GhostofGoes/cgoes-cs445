BIN  = cminus
CC   = g++
SRCS = $(BIN).y $(BIN).l \
cminus.h trees.h toker.h printing.h symbolTable.h semantic_errors.h yyerrorHelper.h types.h codegenClassTM.h \
cminus.cpp trees.cpp toker.cpp printing.cpp symbolTable.cpp semantic_errors.cpp yyerrorHelper.cpp codegenClassTM.cpp
OBJS = lex.yy.o $(BIN).tab.o
LIBS = -lm cminus.cpp toker.cpp symbolTable.cpp printing.cpp trees.cpp semantic_errors.cpp yyerrorHelper.cpp codegenClassTM.cpp
CCFLAGS = -g -std=gnu++11

$(BIN): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) $(LIBS) -o c-

$(BIN).tab.h $(BIN).tab.c: $(BIN).y
	bison -v -t -d $(BIN).y

lex.yy.c: $(BIN).l $(BIN).tab.h
	flex $(BIN).l

all:    
	touch $(SRCS)
	make

clean:
	rm -f c- $(OBJS) $(BIN) lex.yy.c $(BIN).tab.h $(BIN).tab.c $(BIN).tar *~ $(BIN).output

pdf:    $(BIN).y $(BIN).l makefile
	mktex $(BIN).y
	mv $(BIN).pdf $(BIN)-y.pdf
	mktex $(BIN).l
	mv $(BIN).pdf $(BIN)-l.pdf
	mktex makefile
	save $(BIN)-l.pdf  $(BIN)-y.pdf makefile.pdf
	rm *.tex

tar:
	tar -cvf $(BIN).tar $(SRCS) makefile 

tm:
	gcc tm.c -o tm
