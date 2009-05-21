CFLAGS = -Iinclude

all: bin/print_lines bin/print_tokens bin/escape_string bin/list_errors

bin/print_lines: lib/parse.o src/print_lines.c lib/filebuf.o
	cc $(CFLAGS) -o bin/print_lines src/print_lines.c lib/parse.o

bin/print_tokens: lib/parse.o src/print_tokens.c
	cc $(CFLAGS) -o bin/print_tokens src/print_tokens.c lib/parse.o

bin/escape_string: lib/parse.o src/escape_string.c
	cc $(CFLAGS) -o bin/escape_string src/escape_string.c lib/parse.o

bin/list_errors: lib/parse.o src/list_errors.c
	cc $(CFLAGS) -o bin/list_errors src/list_errors.c lib/parse.o
	./bin/list_errors.sh

lib/filebuf.o: src/filebuf.c include/filebuf.h
	cc $(CFLAGS) -c -o lib/filebuf.o src/filebuf.c

lib/parse.o: src/parse.c include/parse.h
	cc $(CFLAGS) -c -o lib/parse.o src/parse.c

clean:
	rm lib/*.o bin/*
