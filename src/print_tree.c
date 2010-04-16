#include <stdio.h>
#include <string.h>
#include "parser.h"

int main(void) {
  struct t_parser p;
  struct t_parser *parser = &p;
  struct t_token *token;
  char *format;

  if (parser_init(parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  do {
    printf("Calling parser_fcall_parse().\n");
    if (!parser_fcall_parse(parser)) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", parser->scanner.error);
      break;
    }

    token = parser_next(parser);
    format = parser_format(parser);
    printf("Parser %s\n", format);

    if (token->type == TT_EOL) {
      token = parser_next(parser);
      if (token->type == TT_NAME) {
        parser_pushtoken(parser);
      }
    }

  } while (token->type != TT_EOF);

  parser_close(parser);
  printf("Done.\n");

  return 0;
}
