CFLAGS = -Wall -Iinclude

all: bin/print_tree bin/print_lines bin/print_chars bin/print_tokens bin/escape_string bin/list_errors bin/exec

bin/print_tree: src/print_tree.c lib/filebuf.o lib/scanner.o lib/parser.o
	cc $(CFLAGS) -o bin/print_tree src/print_tree.c lib/filebuf.o lib/scanner.o lib/parser.o

bin/exec: src/exec.c lib/filebuf.o lib/scanner.o lib/parser.o
	cc $(CFLAGS) -o bin/exec src/exec.c lib/filebuf.o lib/scanner.o lib/parser.o

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

lib/parser.o: src/parser.c include/parser.h
	cc $(CFLAGS) -c -o lib/parser.o src/parser.c

lib/scanner.o: src/scanner.c include/scanner.h
	cc $(CFLAGS) -c -o lib/scanner.o src/scanner.c

clean:
	rm lib/*.o bin/*
