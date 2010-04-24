#ifndef parser_h
#define parser_h

#include "scanner.h"

#define PARSER_FORMAT_BUF_SIZE 1024
#define STATEMENT_FORMAT_BUF_SIZE 1024
#define PARSER_SCRATCH_BUF 1024
#define MAX_FUNC_NAME 100
#define MAX_FUNC_ARGS 20
#define MAX_PARSE_ERRORS 10
#define MAX_ERROR_MSG 200

#define PARSER_ERR_NONE 0
#define PARSER_ERR_MAX_ERRORS 1

#define EXP_NONE    0
#define EXP_ERROR   1
#define EXP_EOF     2
#define EXP_NULL    3
#define EXP_NUM     4
#define EXP_FCALL   5
#define EXP_TERM    6

#define VAL_NULL    0
#define VAL_BOOL    1
#define VAL_INT     2
#define VAL_FLOAT   3
#define VAL_STRING  4
#define VAL_OBJECT  5
#define VAL_VAR     6

#define I_NOP       0
#define I_PUSH      1
#define I_POP       2
#define I_ADD       3
#define I_SUB       4
#define I_MUL       5
#define I_DIV       6
#define I_ASSIGN    7
#define I_EQ        8
#define I_NE        9

extern char *parser_keywords[];
extern char *icodes[];
extern const char *value_types[];
extern int value_types_len;

struct t_parse_error {
  struct t_token token;
};

struct t_parser {
  struct t_scanner scanner;
  struct t_parse_error *errors[MAX_PARSE_ERRORS+2];
  int error;
  struct t_expr *first;
  struct t_expr *stmt;
  //struct list list;
  struct list output;
  char formatbuf[PARSER_FORMAT_BUF_SIZE];
};

/* An expression */
struct t_expr {
  int type;
  char *name;
  int (*init)(struct t_expr *exp);
  int (*copy)(struct t_expr *dest, struct t_expr *source);
  char * (*format)(struct t_expr *exp);
  struct t_expr * (*parse)(struct t_parser *parser);
  int (*destroy)(struct t_expr *exp);
  struct t_expr *prev;
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

struct t_expr_num {
  int type;
  int value;
  char *formatbuf;
};

struct t_func {
  char *name;
  
  /* Native function */
  int (*invoke)(struct t_func *func, struct t_fcall *call, struct t_expr *res);

  /* Local function-- A linked list of statements */
  struct item *first;  
};

struct t_value {
  int type;
  int intval;
  float floatval;
  char *stringval;
  int len;
  char *formatbuf;
};

struct t_icode {
  int type;
  void *operand;
};

/*
 * Term: factor [ ( "*" OR "/" ) ]
 */
struct t_binom {
  struct t_token *sign;
  struct t_expr *left;
  struct t_token *op;
  struct t_expr *right;
  char *formatbuf;
};

/*
 * Parser general
 */
int parser_init(struct t_parser *parser, FILE *in);
int parser_count_errors(struct t_parser *parser);
int parser_close(struct t_parser *parser);
char * parser_format(struct t_parser *parser);

/*
 * Tokens
 */
struct t_token * parser_next(struct t_parser *parser);
struct t_token * parser_token(struct t_parser *parser);
void parser_pushtoken(struct t_parser *parser);
struct t_token * parser_poptoken(struct t_parser *parser);

int parse(struct t_parser *parser);
int parse_stmt(struct t_parser *parser);
int parse_if(struct t_parser *parser);
int parse_assign(struct t_parser *parser);
int compare_multiple_strings(const char *source, char **list);
int parse_expr(struct t_parser *parser);
int parse_simple(struct t_parser *parser);
int parse_term(struct t_parser *parser);
int parse_factor(struct t_parser *parser);
int parse_num(struct t_parser *parser);
int parse_name(struct t_parser *parser);

int create_biop(int type);
int create_push(struct t_value *value);
void value_init(struct t_value *value, int type);
struct t_value * create_num_from_int(int v);
struct t_value * create_num_from_str(char * v);
struct t_value * create_str(char *str);
struct t_value * create_var(char *str);
char * format_value(struct t_value *value);

/*
 * Expressions
 */
int parser_expr_init(struct t_expr *expr, int type);
int parser_addstmt(struct t_parser *parser, struct t_expr *stmt);
int parser_pushexpr(struct t_parser *parser, struct t_expr *expr);
struct t_expr * parser_popexpr(struct t_parser *parser);
char * parser_expr_fmt(struct t_expr *expr);
int parser_expr_destroy(struct t_expr *expr);

/*
 * Parse Expressions
 */
struct t_expr * parser_parse(struct t_parser *parser);
struct t_expr * parser_parse_stmt(struct t_parser *parser);
struct t_expr * parse_parse_assignment(struct t_parser *parser);
struct t_expr * parser_parse_simple(struct t_parser *parser);
struct t_expr * parser_parse_term(struct t_parser *parser);
struct t_expr * parser_parse_factor(struct t_parser *parser);

/*
 * No expression
 */
int parser_none_init(struct t_expr *expr);
char * parser_none_fmt(struct t_expr *expr);

/*
 * fcall: Function call
 */
int parser_fcall_init(struct t_expr *expr);
char * parser_fcall_fmt(struct t_expr *expr);
struct t_expr * parser_fcall_parse(struct t_parser *parser);
int parser_fcall_close(struct t_expr *expr);

/*
 * num: Number primitive
 */
int parser_num_init(struct t_expr *expr);
char * parser_num_fmt(struct t_expr *expr);
struct t_expr * parser_num_parse(struct t_parser *parser);
int parser_num_close(struct t_expr *expr);

/*
 * Binomial
 */
int parser_binom_init(struct t_expr *expr);
char * parser_binom_fmt(struct t_expr *expr);
struct t_expr * parser_binom_parse(struct t_parser *parser);
int parser_binom_close(struct t_expr *expr);

/*
 * Term
 */
int parser_term_init(struct t_expr *expr);
char * parser_term_fmt(struct t_expr *expr);
struct t_expr * parser_term_parse(struct t_parser *parser);
int parser_term_close(struct t_expr *expr);

#endif
