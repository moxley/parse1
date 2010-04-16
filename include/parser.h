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
#define EXP_ERROR   1
#define EXP_EOF     2
#define EXP_FCALL   3

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
  char formatbuf[PARSER_FORMAT_BUF_SIZE];
};

/* An expression */
struct t_expr {
  int type;
  char *name;
  int (*init)(struct t_expr *exp);
  char * (*format)(struct t_expr *exp);
  struct t_expr * (*parse)(struct t_parser *parser);
  int (*destroy)(struct t_expr *exp);
  struct t_expr *next;
  void *detail;
  char *formatbuf;
};

/* Parser error */
struct t_perror {
  char message[MAX_ERROR_MSG];
};

struct t_fcall {
  char *name;
  struct t_expr *firstarg;
  int argcount;
  char *formatbuf;
};

int parser_init(struct t_parser *parser, FILE *in);
int parser_count_errors(struct t_parser *parser);
int parser_close(struct t_parser *parser);
char * parser_format(struct t_parser *parser);
struct t_token * parser_next(struct t_parser *parser);
struct t_token * parser_token(struct t_parser *parser);
void parser_pushtoken(struct t_parser *parser);
int parser_expr_init(struct t_expr *expr, int type);
int parser_addexpr(struct t_parser *parser, struct t_expr *expr);
struct t_expr * parser_expr(struct t_parser *parser);
char * parser_expr_fmt(struct t_expr *expr);
int parser_expr_destroy(struct t_expr *expr);

int parser_none_init(struct t_expr *expr);
char * parser_none_fmt(struct t_expr *expr);

int parser_fcall_init(struct t_expr *expr);
char * parser_fcall_fmt(struct t_expr *expr);
struct t_expr * parser_fcall_parse(struct t_parser *parser);
int parser_fcall_close(struct t_expr *expr);

struct t_expr * parser_expr_parse(struct t_parser *parser);

#endif