CFLAGS = -Wall -Iinclude -g

all: bin/print_tokens bin/escape_string bin/list_errors bin/test_list bin/test_icode bin/format_value bin/test_execstmt bin/test_exec bin/run

bin/run: src/main.c lib/exec.o lib/scanner.o lib/parser.o lib/corelib.o lib/util.o
	cc $(CFLAGS) -o bin/run src/main.c lib/exec.o lib/scanner.o lib/parser.o lib/corelib.o lib/util.o

bin/test_exec: src/test_exec.c lib/exec.o lib/scanner.o lib/corelib.o lib/parser.o lib/util.o
	cc $(CFLAGS) -o bin/test_exec src/test_exec.c lib/exec.o lib/scanner.o lib/corelib.o lib/parser.o lib/util.o

bin/test_execstmt: src/test_execstmt.c lib/exec.o lib/scanner.o lib/parser.o lib/util.o
	cc $(CFLAGS) -o bin/test_execstmt src/test_execstmt.c lib/exec.o lib/scanner.o lib/parser.o lib/util.o

bin/format_value: src/format_value.c lib/scanner.o lib/parser.o lib/util.o
	cc $(CFLAGS) -o bin/format_value src/format_value.c lib/scanner.o lib/parser.o lib/util.o

bin/test_list: src/test_list.c lib/util.o
	cc $(CFLAGS) -o bin/test_list src/test_list.c lib/util.o

bin/test_icode: src/test_icode.c lib/scanner.o lib/parser.o lib/util.o
	cc $(CFLAGS) -o bin/test_icode src/test_icode.c lib/scanner.o lib/parser.o lib/util.o

bin/list_errors: src/list_errors.c lib/scanner.o lib/parser.o lib/util.o
	cc $(CFLAGS) -o bin/list_errors src/list_errors.c lib/scanner.o lib/parser.o lib/util.o

bin/print_tokens: src/print_tokens.c lib/scanner.o lib/util.o
	cc $(CFLAGS) -o bin/print_tokens src/print_tokens.c lib/scanner.o lib/util.o

bin/escape_string: src/escape_string.c lib/scanner.o lib/util.o
	cc $(CFLAGS) -o bin/escape_string src/escape_string.c lib/scanner.o lib/util.o

lib/exec.o: src/exec.c include/exec.h
	cc $(CFLAGS) -c -o lib/exec.o src/exec.c

lib/parser.o: src/parser.c include/parser.h lib/util.o
	cc $(CFLAGS) -c -o lib/parser.o src/parser.c

lib/scanner.o: src/scanner.c include/scanner.h
	cc $(CFLAGS) -c -o lib/scanner.o src/scanner.c

lib/corelib.o: src/corelib.c include/corelib.h
	cc $(CFLAGS) -c -o lib/corelib.o src/corelib.c

lib/util.o: src/util.c include/util.h
	cc $(CFLAGS) -c -o lib/util.o src/util.c

clean:
	rm lib/*.o bin/*
