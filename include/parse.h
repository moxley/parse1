#ifndef parse_h
#define parse_h

#include "scanner.h"

extern char *parser_keywords[];

struct t_parse_error {
  struct t_token token;
};

#define MAX_PARSE_ERRORS 10

struct t_parser {
  struct t_scanner scanner;
  struct t_parse_error *errors[MAX_PARSE_ERRORS+2];
  int error;
};

#define PARSER_ERR_NONE 0
#define PARSER_ERR_MAX_ERRORS 1

int parser_init(struct t_parser *parser, FILE *in);
int parser_count_errors(struct t_parser *parser);
int parser_close(struct t_parser *parser);

#endif
