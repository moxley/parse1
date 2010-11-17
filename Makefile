CFLAGS = -Wall -Iinclude -g
SCANNER_LIBS = lib/scanner.o lib/util.o
PARSER_LIBS = $(SCANNER_LIBS) lib/parser.o
EXEC_LIBS = $(PARSER_LIBS) lib/exec.o lib/corelib.o
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

all: bin/print_tokens bin/escape_string bin/list_errors bin/test_list bin/test_icode bin/format_value bin/test_execstmt bin/test_exec bin/run bin/test_to_s

bin/run: src/main.c $(EXEC_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/test_exec: src/test_exec.c $(EXEC_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/test_execstmt: src/test_execstmt.c lib/exec.o $(EXEC_LIBS)
	cc $(CFLAGS) -o bin/test_execstmt src/test_execstmt.c $(EXEC_LIBS)

bin/test_to_s: src/test_to_s.c $(PARSER_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/format_value: src/format_value.c $(PARSER_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/test_list: src/test_list.c lib/util.o
	cc $(CFLAGS) -o $@ $^

bin/test_icode: src/test_icode.c $(PARSER_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/list_errors: src/list_errors.c $(PARSER_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/print_tokens: src/print_tokens.c $(SCANNER_LIBS)
	cc $(CFLAGS) -o $@ $^

bin/escape_string: src/escape_string.c $(SCANNER_LIBS)
	cc $(CFLAGS) -o $@ $^

lib/exec.o: src/exec.c include/exec.h
	cc $(CFLAGS) -c -o $@ src/exec.c

lib/parser.o: src/parser.c include/parser.h $(SCANNER_LIBS)
	cc $(CFLAGS) -c -o $@ src/parser.c

lib/scanner.o: src/scanner.c include/scanner.h lib/util.o
	cc $(CFLAGS) -c -o $@ src/scanner.c

lib/corelib.o: src/corelib.c include/corelib.h
	cc $(CFLAGS) -c -o $@ src/corelib.c

lib/util.o: src/util.c include/util.h
	cc $(CFLAGS) -c -o $@ src/util.c

clean:
	rm lib/*.o bin/*
