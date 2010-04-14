#include <stdio.h>
#include "parser.h"

int main(void) {
  struct t_parser parser;

  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  do {
    if (parser_p_fcall(&parser)) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", parser.scanner.error);
      break;
    }
    parser_printlast(&parser);
  } while (parser.scanner.token.type != TT_EOF);
  parser_close(&parser);
  printf("Done.\n");
  return 0;
}
