CC=gcc

C_OBJS:=src/lexer.o src/lang.o src/tree.o src/pass.o

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

$(C_OBJS) : src/tree.def src/tree.h

.PHONY: test.c
test.c: test.lc
	./$(EXE) test.lc > test.c

test: $(EXE) test.c
	$(CC) test.c -o test

.PHONY: clean
clean:
	rm -f $(C_OBJS)
	rm -f src/lang.c src/lang.h src/lexer.c
	rm -f src/c/lang.c src/c/lang.h src/c/lexer.c

