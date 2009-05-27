CFLAGS = -Iinclude

all: bin/print_lines bin/print_chars bin/print_tokens bin/escape_string bin/list_errors

bin/print_lines: src/print_lines.c lib/filebuf.o lib/scanner.o
	cc $(CFLAGS) -o bin/print_lines src/print_lines.c lib/filebuf.o lib/scanner.o
	./test/print_lines.sh

bin/print_chars: src/print_chars.c lib/filebuf.o lib/scanner.o
	cc $(CFLAGS) -o bin/print_chars src/print_chars.c lib/filebuf.o lib/scanner.o
	./test/print_chars.sh

bin/print_tokens: src/print_tokens.c lib/scanner.o
	cc $(CFLAGS) -o bin/print_tokens src/print_tokens.c lib/scanner.o
	./test/print_tokens.sh

bin/escape_string: src/escape_string.c lib/scanner.o
	cc $(CFLAGS) -o bin/escape_string src/escape_string.c lib/scanner.o

bin/list_errors: src/list_errors.c lib/scanner.o lib/parse.o
	cc $(CFLAGS) -o bin/list_errors src/list_errors.c lib/scanner.o lib/parse.o
	./test/list_errors.sh

lib/filebuf.o: src/filebuf.c include/filebuf.h
	cc $(CFLAGS) -c -o lib/filebuf.o src/filebuf.c

lib/parse.o: src/parse.c include/parse.h
	cc $(CFLAGS) -c -o lib/parse.o src/parse.c

lib/scanner.o: src/scanner.c include/scanner.h
	cc $(CFLAGS) -c -o lib/scanner.o src/scanner.c

clean:
	rm lib/*.o bin/*
