CFLAGS = -Wall -Iinclude -g

all: bin/print_tree bin/print_lines bin/print_chars bin/print_tokens bin/escape_string bin/list_errors bin/execute

bin/execute: src/execute.c lib/exec.o
	cc $(CFLAGS) -o bin/execute src/execute.c lib/exec.o lib/parser.o lib/scanner.o lib/util.o

bin/print_tree: src/print_tree.c lib/filebuf.o lib/scanner.o lib/parser.o
	cc $(CFLAGS) -o bin/print_tree src/print_tree.c lib/filebuf.o lib/scanner.o lib/parser.o

bin/list_errors: src/list_errors.c lib/scanner.o lib/parser.o
	cc $(CFLAGS) -o bin/list_errors src/list_errors.c lib/scanner.o lib/parser.o

bin/print_lines: src/print_lines.c lib/filebuf.o lib/scanner.o
	cc $(CFLAGS) -o bin/print_lines src/print_lines.c lib/filebuf.o lib/scanner.o

bin/print_chars: src/print_chars.c lib/filebuf.o lib/scanner.o
	cc $(CFLAGS) -o bin/print_chars src/print_chars.c lib/filebuf.o lib/scanner.o

bin/print_tokens: src/print_tokens.c lib/scanner.o
	cc $(CFLAGS) -o bin/print_tokens src/print_tokens.c lib/scanner.o

bin/escape_string: src/escape_string.c lib/scanner.o
	cc $(CFLAGS) -o bin/escape_string src/escape_string.c lib/scanner.o

lib/filebuf.o: src/filebuf.c include/filebuf.h
	cc $(CFLAGS) -c -o lib/filebuf.o src/filebuf.c

lib/exec.o: src/exec.c include/exec.h lib/util.o
	cc $(CFLAGS) -c -o lib/exec.o lib/util.o src/exec.c

lib/parser.o: src/parser.c include/parser.h
	cc $(CFLAGS) -c -o lib/parser.o src/parser.c

lib/scanner.o: src/scanner.c include/scanner.h
	cc $(CFLAGS) -c -o lib/scanner.o src/scanner.c

lib/util.o: src/util.c include/util.h
	cc $(CFLAGS) -c -o lib/util.o src/util.c

clean:
	rm lib/*.o bin/*
