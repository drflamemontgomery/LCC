CC=gcc

C_OBJS:=src/lexer.o src/lang.o src/tree.o

CFLAGS+= -lm -fsanitize=leak -g

EXE:=lcc

default: $(EXE)

src/lang.c src/lang.h: src/lang.y
	bison --output=src/lang.c --header=src/lang.h -Wcounterexamples src/lang.y

src/lexer.c: src/lexer.lex src/lang.h
	flex -o src/lexer.c src/lexer.lex


%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXE): $(C_OBJS)
	$(CC) $(CFLAGS) $(C_OBJS) -o $(EXE)

.PHONY: clean
clean:
	find . -name "*.o" -exec rm {} \;

