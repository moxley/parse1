#ifndef parser_h
#define parser_h

#include "scanner.h"

#define PARSER_FORMAT_BUF_SIZE 1024
#define STATEMENT_FORMAT_BUF_SIZE 1024
#define MAX_FUNC_NAME 100
#define MAX_FUNC_ARGS 20
#define MAX_PARSE_ERRORS 10
#define MAX_ERROR_MSG 200

#define PARSER_ERR_NONE 0
#define PARSER_ERR_MAX_ERRORS 1

#define EXP_NONE    0
#define EXP_FCALL   1

extern char *parser_keywords[];

struct t_parse_error {
  struct t_token token;
};

struct t_parser {
  struct t_scanner scanner;
  struct t_parse_error *errors[MAX_PARSE_ERRORS+2];
  int error;
  struct t_expr *first;
  struct t_expr *expr;
  char format[PARSER_FORMAT_BUF_SIZE];
};

/* An expression */
struct t_expr {
  int type;
  char * (*fmt)(struct t_expr *exp);
  struct t_expr *next;
  char format[STATEMENT_FORMAT_BUF_SIZE];
};

/* Parser error */
struct t_perror {
  int type;
  char * (*fmt)(struct t_expr *exp);
  struct t_expr *next;
  char format[STATEMENT_FORMAT_BUF_SIZE];
  char message[MAX_ERROR_MSG];
};

struct t_fcall {
  int type;
  char * (*fmt)(struct t_expr *exp);
  struct t_expr *next;
  char format[STATEMENT_FORMAT_BUF_SIZE];
  char name[MAX_FUNC_NAME];
  char *args[MAX_FUNC_ARGS];
};

int parser_init(struct t_parser *parser, FILE *in);
int parser_count_errors(struct t_parser *parser);
int parser_close(struct t_parser *parser);
int parser_format(struct t_parser *parser);
struct t_token * parser_next(struct t_parser *parser);
int parser_addexpr(struct t_parser *parser, struct t_expr *expr);

char * parser_none_fmt(struct t_expr *expr);

int parser_fcall_init(struct t_fcall *call);
char * parser_fcall_fmt(struct t_expr *expr);
struct t_fcall * parser_fcall_parse(struct t_parser *parser);
#endif
