#include <stdio.h>
#include <string.h>
#include "parser.h"

int main(void) {
  struct t_parser parser;
  struct t_fcall call;

  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  strcpy(call.name, "myfunc");
  parser_fcall_init(&call);
  printf("%s\n", parser_fcall_fmt((struct t_expr *) &call));

  parser_format(&parser);
  printf("%s\n", parser.format);

  parser_close(&parser);
  return 0;

  do {
    if (parser_fcall_parse(&parser)) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", parser.scanner.error);
      break;
    }
    parser_fcall_fmt(parser.expr);
  } while (parser.scanner.token.type != TT_EOF);
  parser_close(&parser);
  printf("Done.\n");
  return 0;
}
