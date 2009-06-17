#include "parser.h"

/* Keywords */
char *parser_keywords[] = {
  "print",
  (char *) 0
};

char *parser_error_names[] = {
  "PARSER_ERR_NONE",
  "PARSER_ERR_MAX_ERRORS"
};

int parser_init(struct t_parser *parser, FILE *in) {
  parser->errors[0] = (struct t_parse_error *) 0;
  parser->error = PARSER_ERR_NONE;
  if (scanner_init(&(parser->scanner), in)) return 1;
  return 0;
}

int parser_count_errors(struct t_parser *parser) {
  int error_i = 0;
  
  do {
    if (scanner_token(&(parser->scanner))) return 1;
    if (parser->scanner.token.type == TT_ERROR) {
      parser->errors[error_i] = malloc(sizeof(struct t_parse_error));
      if (error_i == MAX_PARSE_ERRORS) {
	token_init(&(parser->errors[MAX_PARSE_ERRORS]->token), TT_ERROR);
	parser->errors[MAX_PARSE_ERRORS]->token.error = PERR_MAX_ERRORS;
	break;
      }
      else {
	token_copy(&(parser->errors[error_i]->token), &(parser->scanner.token));
      }
      error_i++;
    }
  } while(parser->scanner.token.type != TT_EOF);

  parser->errors[error_i] = (struct t_parse_error *) 0;
  return parser->error;
}

int parser_close(struct t_parser *parser) {
  int i;
  scanner_close(&(parser->scanner));
  for (i=0; parser->errors[i]; i++) {
    free(parser->errors[i]);
  }
  return 0;
}
