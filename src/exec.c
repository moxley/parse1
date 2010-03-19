#include "scanner.h"
#include "parser.h"

struct t_function {
  char *name;
};

int exec_run(struct t_scanner *scanner);
int exec_func(struct t_scanner *scanner);
int parser_parse_func(struct t_scanner *scanner, struct t_function *func); 
int parser_parse_func_args(struct t_scanner *scanner, struct t_function *func);

int main(int argc, char** argv) {
  struct t_scanner scanner;

  if (scanner_init(&scanner, stdin)) {
    fprintf(stderr, "Failed to initialize scanner\n");
    return 1;
  }
  scanner.debug = 1;
  exec_run(&scanner);
  scanner_close(&scanner);
  printf("Done.\n");
  return 0;
}

int exec_run(struct t_scanner *scanner) {
  do {
    if (scanner_token(scanner)) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", scanner->error);
      return 1;
    }
	if (scanner->token.type == TT_NAME) {
	  if (exec_func(scanner)) return 1;
	}
  } while (scanner->token.type != TT_EOF);
  return 0;
}

int exec_func(struct t_scanner *scanner) {
  struct t_function func;
  if (parser_parse_func(scanner, &func)) return 1;
  return 0;
}

int parser_parse_func(struct t_scanner *scanner, struct t_function *func) {
  printf("Function name: %s\n", scanner->token.buf);
  if (scanner_token(scanner)) return 1;
  if (parser_parse_func_args(scanner, func)) return 1;
  return 0;
}

int parser_parse_func_args(struct t_scanner *scanner, struct t_function *func) {
  do {
	if (scanner->token.type != TT_NUM) {
	  if (scanner->token.type == TT_EOL) {
		printf("EOL found. Scanning for next token\n");
	    if (scanner_token(scanner)) return 1;
	  }
	  break;
	}
	else {
		printf("Number: %s\n", scanner->token.buf);
	    if (scanner_token(scanner)) return 1;
	}
  } while (1);
  return 0;
}

