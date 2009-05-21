#include <stdio.h>
#include "parse.h"

#define BUF_SIZE 30

int main(void) {
  struct t_parser parser;
  char buf[BUF_SIZE];
  int result;
  int i;

  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  if (parser_count_errors(&parser)) {
    fprintf(stderr, "Failure while counting errors\n");
  }
  else {
    for (i=0; parser.errors[i]; i++) {
      printf("Error %d: %s\n", i+1, parse_error_names[parser.errors[i]->token.error]);
    }
  }

  parser_close(&parser);
  printf("Done.\n");

  return 0;
}
