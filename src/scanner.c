#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "scanner.h"

#define CC_TABLE_SIZE 256
int scanner_cc_table[CC_TABLE_SIZE];
int scanner_cc_table_initialized = 0;

char *error_names[] = {
  "ERR_NONE",
  "ERR_READ",
  "ERR_WRITE"
};

char *parse_error_names[] = {
  "PERR_NONE",
  "PERR_MAX_TOKEN_SIZE",
  "PERR_MAX_NUM_SIZE",
  "PERR_MAX_ERRORS"
};

char *token_types[] = {
  "TT_UNKNOWN",
  "TT_ERROR",
  "TT_EOL",
  "TT_EOF",
  "TT_NUM",
  "TT_NAME",
  "TT_EQUAL",
  "TT_PLUS",
  "TT_PARENL",
  "TT_PARENR",
  "TT_COMMA"
};

char * scanner_cc_names[] = {
  "CC_UNKNOWN",
  "CC_EOL",
  "CC_EOF",
  "CC_SPACE",
  "CC_DIGIT",
  "CC_ALPHA",
  "CC_OP",
  "CC_DELIM",
  "CC_QUOTE"
};

char *scanner_operators = "~!@%^&*-+=|?/";
char *scanner_delimiters = "()[]{}.:,";
char *scanner_quotes = "\"'`";

int scanner_init(struct t_scanner *scanner, FILE *in) {
  scanner->c = '\0';
  scanner->c_class = CC_UNKNOWN;
  scanner->reuse = 0;
  scanner->error = ERR_NONE;
  scanner->row = 0;
  scanner->col = -1;
  scanner->debug = 0;
  scanner->found_eol = 0;
  scanner->in = in;
  scanner->token = NULL;
  scanner->first = NULL;
  scanner->token_count = 0;
  scanner->stack_size = 0;
  
  if (!scanner_cc_table_initialized) {
    scanner_build_cc_table();
    scanner_cc_table_initialized = 1;
  }

  if (scanner_getc(scanner)) {
    scanner->error = ERR_READ;
    return 1;
  }

  _scanner_init_token(scanner, &scanner->unknown, TT_UNKNOWN);

  return 0;
}

void _scanner_init_token(struct t_scanner *scanner, struct t_token *token, int type) {
  token->type = type;
  token->buf_i = 0;
  token->buf = NULL;
  token->error = PERR_NONE;
  token->row = scanner->row;
  token->col = scanner->col;
  token->formatbuf = NULL;
  token->prev = NULL;
  token->next = NULL;
}

struct t_token * scanner_init_token(struct t_scanner *scanner, int type) {
  struct t_token *token;
  
  token = malloc(sizeof(struct t_token));
  _scanner_init_token(scanner, token, type);

  if (scanner->token) {
    scanner->token->next = token;
    token->prev = scanner->token;
  }
  else {
    scanner->first = token;
  }
  
  scanner->token = token;
  scanner->token_count++;
  
  return scanner->token;
}

void token_copy(struct t_token *dest, const struct t_token *source) {
  dest->buf_i = source->buf_i;
  strcpy(dest->buf, source->buf);
  dest->type = source->type;
  dest->error = source->error;
  dest->row = source->row;
  dest->col = source->col;
  if (dest->formatbuf) free(dest->formatbuf);
  dest->formatbuf = malloc(sizeof(char) * (strlen(source->formatbuf) + 1));
  strcpy(dest->formatbuf, source->formatbuf);
}

void scanner_close(struct t_scanner *scanner) {
  struct t_token *t;
  struct t_token *next;
  
  fclose(scanner->in);
  
  t = scanner->first;
  while (t) {
    next = t->next;
    free(t);
    t = next;
  }
}

int scanner_getc(struct t_scanner *scanner) {
  char esc_char[3];
  
  if (scanner->reuse) {
    scanner->reuse = 0;
  }
  else {
    scanner->c = getc(scanner->in);
    scanner->col++;
    if (scanner->c == '\r') {
      if (scanner->found_eol) {
        scanner->row++;
        scanner->col = 0;
      }
      else {
        scanner->found_eol = 1;
      }
    }
    else if (scanner->c == '\n') {
      scanner->found_eol = 1;
    }
    else if (scanner->found_eol) {
      scanner->row++;
      scanner->col = 0;
      scanner->found_eol = 0;
    }
    scanner->c_class = scanner_charclass(scanner->c);
  }

  if (scanner->debug >= 2) {
    util_escape_char(esc_char, scanner->c);
    printf("scanner_getc() c: '%s', c_class: %s\n",
      esc_char,
      scanner_cc_names[scanner->c_class]);
  }
  
  return 0;
}

char * scanner_format(struct t_scanner *scanner) {
  char esc_char[3];
  char buf[SCRATCH_BUF_SIZE + 1];
  int len;
  char *toobig = "<#scanner: TOO_BIG>";
  
  util_escape_char(esc_char, scanner->c);
  len = snprintf(buf, SCRATCH_BUF_SIZE, "<#scanner: {token: %s, first: %s, row: %d, col: '%d', debug: %d, found_eol: %d, token_count: %d, stack_size: %d, c: '%s', c_class: %s, reuse: %d, error: %d}>",
     token_format(scanner->token),
     token_format(scanner->first),
     scanner->row,
     scanner->col,
     scanner->debug,
     scanner->found_eol,
     scanner->token_count,
     scanner->stack_size,
     esc_char,
     scanner_cc_names[scanner->c_class],
     scanner->reuse,
     scanner->error);
  if (scanner->formatbuf) free(scanner->formatbuf);
  if (len > SCRATCH_BUF_SIZE) {
    scanner->formatbuf = malloc(sizeof(char) * (strlen(toobig) + 1));
    strcpy(scanner->formatbuf, toobig);
  }
  else {
    scanner->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(scanner->formatbuf, buf);
  }
  
  
  return scanner->formatbuf;
}

void scanner_print(struct t_scanner *scanner) {
  puts(scanner_format(scanner));
}

char * token_format(struct t_token *token) {
  int len;
  char esc_buf[SCRATCH_BUF_SIZE + 1];
  char buf[SCRATCH_BUF_SIZE + 1];
  char *toobig = "<#token TOO_BIG>";
  
  util_escape_string(esc_buf, SCRATCH_BUF_SIZE, token->buf);
  len = snprintf(buf,
           SCRATCH_BUF_SIZE,
           "<#token {type: %s, error: %s, buf: '%s'}>",
           token_types[token->type],
           parse_error_names[token->error],
           esc_buf);
  if (token->formatbuf) free(token->formatbuf);
  if (len > SCRATCH_BUF_SIZE) {
    token->formatbuf = malloc(sizeof(char) * (strlen(toobig) + 1));
    strcpy(token->formatbuf, toobig);
  }
  else {
    token->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(token->formatbuf, buf);
  }
  
  return token->formatbuf;
}

struct t_token * scanner_next(struct t_scanner *scanner) {
  struct t_token *token;
  int i;
  
  if (scanner->stack_size > 0) {
    token = scanner->token;
    for (i=1; i < scanner->stack_size; i++) {
      token = token->prev;
    }
    scanner->stack_size--;
    return token;
  }
  
  if (scanner_skip_whitespace(scanner)) return NULL;

  if (scanner->c_class == CC_EOF) {
    token = scanner_init_token(scanner, TT_EOF);
  }
  else if (scanner->c_class == CC_EOL) {
    token = scanner_init_token(scanner, TT_EOL);
    scanner_token_char(scanner);
    if (scanner_getc(scanner)) return NULL;
  }
  else if (scanner->c_class == CC_DIGIT) {
    token = scanner_parse_num(scanner);
  }
  else if (scanner->c_class == CC_ALPHA) {
    token = scanner_parse_name(scanner);
  }
  else if (scanner->c_class == CC_OP) {
    token = scanner_parse_op(scanner);
  }
  else if (scanner->c_class == CC_DELIM) {
    token = scanner_parse_delim(scanner);
  }
  else {
    token = scanner_init_token(scanner, TT_UNKNOWN);
    scanner_token_char(scanner);
    if (scanner_getc(scanner)) return NULL;
    if (scanner->debug) {
      scanner_print(scanner);
    }
  }
  return token;
}

/*
 * Allocate a token buf and copy the current scanner char to it.
 */
char * scanner_token_char(struct t_scanner *scanner) {
  struct t_token *token;
  
  token = scanner->token;
  if (token->buf) free(token->buf);
  token->buf = malloc(sizeof(char) * 2);
  token->buf[0] = scanner->c;
  token->buf[1] = '\0';
  
  return token->buf;
}

struct t_token * scanner_token(struct t_scanner *scanner) {
  if (!scanner->token) {
    return &scanner->unknown;
  }
  else {
    return scanner->token;
  }
}

/*
 * Push the token back into the stream.
 */
void scanner_push(struct t_scanner *scanner) {
  scanner->stack_size++;
}

struct t_token * scanner_parse_num(struct t_scanner *scanner) {
  int i;
  struct t_token *token;
  char buf[SCRATCH_BUF_SIZE + 1];
  int buf_i = 0;
  
  token = scanner_init_token(scanner, TT_NUM);
  for (i=0; 1; ++i) {
    if (i >= MAX_NUM_LEN) {
      token->type = TT_ERROR;
      token->error = PERR_MAX_NUM_SIZE;
    }
    buf[buf_i++] = scanner->c;
    if (scanner_getc(scanner)) return NULL;
    if (scanner->c_class != CC_DIGIT) {
      break;
    }
  }
  buf[buf_i] = '\0';
  if (token->buf) free(token->buf);
  token->buf = malloc(sizeof(char) * (buf_i + 1));
  strcpy(token->buf, buf);
  
  return token;
}

struct t_token * scanner_parse_name(struct t_scanner *scanner) {
  struct t_token *token;
  char buf[SCRATCH_BUF_SIZE + 1];
  int buf_i = 0;
  
  token = scanner_init_token(scanner, TT_NAME);
  while (1) {
    buf[buf_i++] = scanner->c;
    if (scanner_getc(scanner)) return NULL;
    if (!isalpha(scanner->c)) {
      break;
    }
  }
  buf[buf_i] = '\0';
  if (token->buf) free(token->buf);
  token->buf = malloc(sizeof(char) * (buf_i + 1));
  strcpy(token->buf, buf);
  
  return token;
}

struct t_token * scanner_parse_op(struct t_scanner *scanner) {
  struct t_token *token;
  char buf[SCRATCH_BUF_SIZE + 1];
  int buf_i = 0;
  
  if (scanner->c == '+') {
    token = scanner_init_token(scanner, TT_PLUS);
  }
  else if (scanner->c == '=') {
    token = scanner_init_token(scanner, TT_EQUAL);
  }
  else {
    token = scanner_init_token(scanner, TT_UNKNOWN);
    return token;
  }
  int i;
  int invalid = 0;
  for (i=0; 1; i++) {
    buf[buf_i++] = scanner->c;
    if (scanner_getc(scanner)) return NULL;
    if (scanner->c_class == CC_OP) {
      invalid = 1;
    }
    else {
      break;
    }
  }
  
  buf[buf_i] = '\0';
  if (token->buf) free(token->buf);
  token->buf = malloc(sizeof(char) * (buf_i + 1));
  strcpy(token->buf, buf);

  if (invalid) {
    token->type = TT_UNKNOWN;
  }

  return token;
}

struct t_token * scanner_parse_delim(struct t_scanner *scanner)
{
  int type = TT_UNKNOWN;
  struct t_token *token;
  
  switch (scanner->c) {
  case '(':
    type = TT_PARENL;
    break;
  case ')':
    type = TT_PARENR;
    break;
  case ',':
    type = TT_COMMA;
    break;
  }

  token = scanner_init_token(scanner, type);
  if (token->buf) free(token->buf);
  token->buf = malloc(sizeof(char) * 2);
  token->buf[0] = scanner->c;
  token->buf[1] = '\0';
  if (scanner_getc(scanner)) return NULL;
  return token;
}

//int token_append(struct t_scanner *scanner)
//{
//  struct t_token *token;
//  
//  token = scanner_token(scanner);
//  if (token->buf_i >= TOKEN_BUF_SIZE) {
//    scanner->error = PERR_MAX_TOKEN_SIZE;
//    return 1;
//  }
//  token->buf[token->buf_i] = scanner->c;
//  token->buf[token->buf_i+1] = '\0';
//  token->buf_i++;
//  return 0;
//}

int scanner_skip_whitespace(struct t_scanner *scanner) {
  while (scanner->c_class == CC_SPACE) {
    if (scanner_getc(scanner)) return 1;
  }
  return 0;
}

int scanner_charclass(int c) {
  if (c == EOF) {
    return CC_EOF;
  }
  else if (c >= CC_TABLE_SIZE) {
    return CC_UNKNOWN;
  }
  else {
    return scanner_cc_table[c];
  }
}

void scanner_build_cc_table() {
  int i;
  for (i=0; i < CC_TABLE_SIZE; ++i) {
    if (i == EOF) {
      scanner_cc_table[i] = CC_EOF;
    }
    else if (isdigit(i)) {
      scanner_cc_table[i] = CC_DIGIT;
    }
    else if (isalpha(i)) {
      scanner_cc_table[i] = CC_ALPHA;
    }
    else if (i == '\r' || i == '\n') {
      scanner_cc_table[i] = CC_EOL;
    }
    else if (isspace(i)) {
      scanner_cc_table[i] = CC_SPACE;
    }
    else if (strchr(scanner_operators, i)) {
      scanner_cc_table[i] = CC_OP;
    }
    else if (strchr(scanner_quotes, i)) {
      scanner_cc_table[i] = CC_QUOTE;
    }
    else if (strchr(scanner_delimiters, i)) {
      scanner_cc_table[i] = CC_DELIM;
    }
    else {
      scanner_cc_table[i] = CC_UNKNOWN;
    }

  }
}

/**
 * Escape special characters as required by a C-formatted string.
 */
int util_escape_string(char *buf, int buf_size, const char *str) {
  int i;
  int buf_i = 0;
  int out_of_bounds = 0;
  char esc_buf[3];
  int esc_len;
  
  for (i=0; str[i]; ++i) {
    esc_len = util_escape_char(esc_buf, str[i]);
    if (buf_i + esc_len < buf_size-1) {
      strcpy(buf + buf_i, esc_buf);
      buf_i += esc_len;
    }
    else {
      out_of_bounds = 1;
      break;
    }
  }
  buf[buf_i] = '\0';
  return out_of_bounds;
}

int util_escape_char(char *buf, char c) {
  int buf_i = 0;
  if (c == '\'' || c == '"' || c == '\\') {
    buf[buf_i++] = '\\';
    buf[buf_i++] = c;
  }
  else if (c == '\r') {
    buf[buf_i++] = '\\';
    buf[buf_i++] = 'r';
  }
  else if (c == '\n') {
    buf[buf_i++] = '\\';
    buf[buf_i++] = 'n';
  }
  else if (c == '\t') {
    buf[buf_i++] = '\\';
    buf[buf_i++] = 't';
  }
  else {
    buf[buf_i++] = c;
  }
  buf[buf_i] = '\0';
  return buf_i;
}
