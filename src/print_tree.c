#include <stdio.h>
#include <string.h>
#include "parser.h"

extern char * nullexpr;

int main(void) {
  struct t_parser p;
  struct t_parser *parser = &p;
  struct t_token *token;
  struct t_expr *expr;

  if (parser_init(parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  do {
    token = parser_next(parser);
    if (token->type != TT_NAME) {
      fprintf(stderr, "Expected TT_NAME. Got: %s\n", token_format(token));
      break;
    }
    
    expr = parser_fcall_parse(parser);
    if (!expr) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", parser->scanner.error);
      break;
    }
    parser_addstmt(parser, expr);
    printf("Statement: %s\n", parser_expr_fmt(expr));
    token = parser_token(parser);

    while (token->type != TT_NAME && token->type != TT_EOF) {
      token = parser_next(parser);
      if (!token) {
        fprintf(stderr, "Failed to get next token\n");
        exit(1);
      }
      else if (token->type == TT_NAME) {
        parser_pushtoken(parser);
      }
    }

  } while (token->type != TT_EOF);

  parser_close(parser);
  printf("Done.\n");

  return 0;
}
