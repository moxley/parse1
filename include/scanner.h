#ifndef scanner_h
#define scanner_h

#include <stdio.h>
#include <stdlib.h>
#include "util.h"

#define ERR_NONE  0
#define ERR_READ  1
#define ERR_WRITE 2

extern char *error_names[];

/* Parse errors */
#define PERR_NONE            0
#define PERR_MAX_TOKEN_SIZE  1
#define PERR_MAX_NUM_SIZE    2
#define PERR_MAX_NAME_SIZE   3
#define PERR_MAX_ERRORS      4
#define PERR_MAX_STRING_SIZE 5
#define PERR_UNEXPECTED_EOF  6

extern char *parse_error_names[];

#define SCRATCH_BUF_SIZE 1000

#define TT_UNKNOWN 0
#define TT_ERROR   1
#define TT_EOL     2
#define TT_EOF     3
#define TT_NUM     4
#define TT_NAME    5
#define TT_EQUAL   6
#define TT_PLUS    7
#define TT_PARENL  8
#define TT_PARENR  9
#define TT_COMMA   10
#define TT_SEMI    11
#define TT_STRING  12
#define TT_LT      13
#define TT_GT      14

extern char *token_types[];

/* Character classes */
#define CC_UNKNOWN 0
#define CC_EOL     1
#define CC_EOF     2
#define CC_SPACE   3
#define CC_DIGIT   4
#define CC_ALPHA   5
#define CC_OP      6
#define CC_DELIM   7
#define CC_QUOTE   8

extern char * scanner_cc_names[];

#define MAX_NUM_LEN 4
#define MAX_NAME_LEN 50
#define MAX_STRING_LEN 50

extern struct t_indent indent;

struct t_char {
  int c;
  int c_class;
  int row;
  int col;
  char *formatbuf;
};

struct t_token {
  int type;
  char *buf;
  int error;
  int row;
  int col;
  char *formatbuf;
};

struct t_scanner {
  FILE *in;
  int error;
  int debug;
  struct t_char *current;
  struct t_token *token;  // Last of all tokens, and the current token
  struct t_token unknown;
  struct list c_list;
  struct list c_pushback;
  struct list t_list;
  struct list t_pushback;
  char *formatbuf;
};

int scanner_init(struct t_scanner *scanner, FILE *in);
void scanner_close(struct t_scanner *scanner);

struct t_token * scanner_create_token(struct t_scanner *scanner, int type);
void scanner_init_token(struct t_scanner *scanner, struct t_token *token, int type);
void token_copy(struct t_token *dest, const struct t_token *source);

struct t_char * scanner_c(struct t_scanner *scanner);
int scanner_ch(struct t_scanner *scanner);
struct t_char * scanner_nextc(struct t_scanner *scanner);
int scanner_nextch(struct t_scanner *scanner);
int scanner_pushc(struct t_scanner *scanner);

void scanner_print(struct t_scanner *scanner);
char * scanner_format(struct t_scanner *scanner);
char * char_format(struct t_char *ch);
char * token_format(struct t_token *token);

struct t_token * scanner_next(struct t_scanner *scanner);
char * scanner_token_char(struct t_scanner *scanner);
void scanner_push(struct t_scanner *scanner);
struct t_token * scanner_pop(struct t_scanner *scanner);
struct t_token * scanner_token(struct t_scanner *scanner);

struct t_token * scanner_parse_eol(struct t_scanner *scanner);
struct t_token * scanner_parse_num(struct t_scanner *scanner);
struct t_token * scanner_parse_name(struct t_scanner *scanner);
struct t_token * scanner_parse_op(struct t_scanner *scanner);
struct t_token * scanner_parse_delim(struct t_scanner *scanner);
struct t_token * scanner_parse_string(struct t_scanner *scanner);

//int token_append(struct t_scanner *scanner);
int scanner_skip_whitespace(struct t_scanner *scanner);

int scanner_charclass(int c);
void scanner_build_cc_table();
int util_escape_string(char buf[], int buf_size, const char *str);
int util_escape_char(char *buf, char c);

#endif
