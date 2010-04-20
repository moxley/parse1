#include <stdio.h>
#include "parser.h"

int main(void) {
  struct t_parser parser;
  struct t_token *token;
  struct t_expr *expr;

  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  do {
    printf("Parsing statement:\n");
    expr = parser_expr_parse(&parser);
    if (!expr) {
      fprintf(stderr, "  Failed to get expression\n");
      break;
    }
    
    printf("  Statement: %s\n", parser_expr_fmt(expr));
    token = parser_token(&parser);

    while (token->type == TT_EOL) {
      token = parser_next(&parser);
      if (!token) {
        fprintf(stderr, "  Failed to get next token\n");
        exit(1);
      }
    }
    printf("\n");

  } while (token->type != TT_EOF);

  parser_close(&parser);
  printf("Done.\n");

  return 0;
}
