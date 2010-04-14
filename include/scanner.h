#ifndef scanner_h
#define scanner_h

#include <stdio.h>
#include <stdlib.h>

#define ERR_NONE  0
#define ERR_READ  1
#define ERR_WRITE 2

extern char *error_names[];

/* Parse errors */
#define PERR_NONE 0
#define PERR_MAX_TOKEN_SIZE 1
#define PERR_MAX_NUM_SIZE 2
#define PERR_MAX_ERRORS 3

extern char *parse_error_names[];

#define SCRATCH_BUF_SIZE 1000
#define TOKEN_BUF_SIZE 200
#define TOKEN_FORMAT_BUF_SIZE 500
#define SCANNER_FORMAT_BUF_SIZE 1000

#define TT_UNKNOWN 0
#define TT_ERROR   1
#define TT_EOF     2
#define TT_NUM     3
#define TT_NAME    4
#define TT_EQUAL   5
#define TT_PLUS    6
#define TT_PARENL  7
#define TT_PARENR  8

extern char *token_types[];

/* Character classes */
#define CC_UNKNOWN 0
#define CC_EOF     1
#define CC_SPACE   2
#define CC_DIGIT   3
#define CC_ALPHA   4
#define CC_OP      5
#define CC_DELIM   6
#define CC_QUOTE   7

extern char * scanner_cc_names[];

#define MAX_NUM_LEN 4

struct t_token {
  int type;
  char buf[TOKEN_BUF_SIZE];
  int buf_i;
  int error;
  int row;
  int col;
  char format[TOKEN_FORMAT_BUF_SIZE];
};

struct t_scanner {
  int c;
  int c_class;
  int reuse;
  FILE *in;
  int error;
  int row;
  int col;
  int found_eol;
  struct t_token token;
  char format[SCANNER_FORMAT_BUF_SIZE];
};

int scanner_init(struct t_scanner *scanner, FILE *in);
void scanner_init_token(struct t_scanner *scanner, int type);
void token_copy(struct t_token *dest, const struct t_token *source);
void scanner_close(struct t_scanner *scanner);

int scanner_getc(struct t_scanner *scanner);

void scanner_print(struct t_scanner *scanner);
int scanner_format(struct t_scanner *scanner);
int token_format(struct t_token *token);

int scanner_token(struct t_scanner *scanner);

int scanner_token_num(struct t_scanner *scanner);
int scanner_token_name(struct t_scanner *scanner);
int scanner_token_op(struct t_scanner *scanner);
int scanner_token_delim(struct t_scanner *scanner);

int token_append(struct t_scanner *scanner);
int scanner_skip_whitespace(struct t_scanner *scanner);

int scanner_charclass(int c);
void scanner_build_cc_table();
int util_escape_string(char *buf, int buf_size, const char *str);
int util_escape_char(char *buf, char c);

#endif
