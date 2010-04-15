#include <stdio.h>
#include <string.h>
#include "parser.h"

void testprint(struct t_parser *parser);

int main(void) {
  struct t_parser p;
  struct t_parser *parser = &p;
  struct t_token *token;

  if (parser_init(parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  if (0) {
	testprint(parser);
  }
  else {
    do {
	  printf("Calling parser_fcall_parse().\n");
      if (!parser_fcall_parse(parser)) {
        fprintf(stderr, "An error occurred during parsing: errno: %d\n", parser->scanner.error);
        break;
      }
	  printf("parser.expr.type: %d\n", parser->expr->type);
      parser_fcall_fmt(parser->expr);

	  token = parser_token(parser);
      parser_format(parser);
	  printf("%s\n", parser->format);

	  if (token->type == TT_EOL) {
		token = parser_token(parser);
	  }

    } while (token->type != TT_EOF);
  }

  parser_close(parser);
  printf("Done.\n");

  return 0;
}

void testprint(struct t_parser *parser) {
  struct t_fcall call;

  /*
   * Print a function call
   */
  strcpy(call.name, "myfunc");
  parser_fcall_init(&call);
  printf("%s\n", parser_fcall_fmt((struct t_expr *) &call));

  /*
   * Assign the function call to the parser
   */
  parser->expr = (struct t_expr *) &call;

  /*
   * Print the parser
   */
  parser_format(parser);
  printf("%s\n", parser->format);
}

