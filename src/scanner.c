#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "scanner.h"
#include "util.h"

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
  "PERR_MAX_NAME_SIZE",
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
  memset(scanner, 0, sizeof(struct t_scanner));
  scanner->in = in;
  
  if (!scanner_cc_table_initialized) {
    scanner_build_cc_table();
    scanner_cc_table_initialized = 1;
  }

  _scanner_init_token(scanner, &scanner->unknown, TT_UNKNOWN);

  return 0;
}

void _scanner_init_token(struct t_scanner *scanner, struct t_token *token, int type) {
  struct t_char *c;
  
  c = scanner_c(scanner);
  memset(token, 0, sizeof(struct t_token));
  token->type = type;
  token->row = c->row;
  token->col = c->col;
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
  dest->type = source->type;
  if (dest->buf) free(dest->buf);
  if (!source->buf) {
	dest->buf = NULL;
  }
  else {
    dest->buf = malloc(sizeof(char) * (strlen(source->buf) + 1));
    strcpy(dest->buf, source->buf);
  }
  dest->buf_i = source->buf_i;
  dest->error = source->error;
  dest->row = source->row;
  dest->col = source->col;
  dest->prev = source->prev;
  dest->next = source->next;
  if (dest->formatbuf) free(dest->formatbuf);
  if (!source->formatbuf) {
	dest->formatbuf = NULL;
  }
  else {
    dest->formatbuf = malloc(sizeof(char) * (strlen(source->formatbuf) + 1));
    strcpy(dest->formatbuf, source->formatbuf);
  }
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

int scanner_nextch(struct t_scanner *scanner) {
  return scanner_nextc(scanner)->c;
}

struct t_char * scanner_nextc(struct t_scanner *scanner) {
  struct t_char *c;
  struct item *top;
  
  if (scanner->pushback) {
    top = scanner->pushback;
    scanner->pushback = top->prev;
    if (scanner->pushback) {
      scanner->pushback->next = NULL;
    }
    scanner->current = (struct t_char *) top->value;
    free(top);
  }
  else {
    c = malloc(sizeof(struct t_char));
	memset(c, 0, sizeof(struct t_char));
    c->c = getc(scanner->in);
    if (!scanner->current || (scanner->current->c == '\r' && c->c != '\n') || scanner->current->c == '\n') {
      c->col = 0;
      c->row = 0;
    }
    else {
      c->col = scanner->current->col + 1;
      c->row = scanner->current->row;
    }
    c->c_class = scanner_charclass(c->c);
    free(scanner->current);
    scanner->current = c;
  }
  
  return scanner->current;
}

/*
 * Get the current character
 */
struct t_char * scanner_c(struct t_scanner *scanner) {
  if (!scanner->current) {
    return scanner_nextc(scanner);
  }
  else {
    return scanner->current;
  }
}

int scanner_ch(struct t_scanner *scanner) {
  return scanner_c(scanner)->c;
}

/*
 * Push the current character back into the stream.
 */
int scanner_pushc(struct t_scanner *scanner) {
  struct item *item;
  
  item = llist_newitem(scanner->current);
  if (!scanner->pushback) {
    scanner->pushback = item;
  }
  else {
    scanner->pushback->next = item;
    item->prev = scanner->pushback;
    scanner->pushback = item;
  }
  
  return 0;
}

char * char_format(struct t_char *ch) {
  char esc_char[3];
  char buf[SCRATCH_BUF_SIZE + 1];
  int len;
  int allocsize;
  char *source;
  char *toobig = "<#t_char: TOO_BIG>";
  
  util_escape_char(esc_char, ch->c);
  len = snprintf(buf, SCRATCH_BUF_SIZE, "<#t_char: {c: '%s', c_class: %s, row: %d, col: %d}>",
    esc_char,
    scanner_cc_names[ch->c_class],
    ch->row,
    ch->col);
  if (len > SCRATCH_BUF_SIZE) {
    allocsize = strlen(toobig) + 1;
    source = toobig;
  }
  else {
    allocsize = len + 1;
    source = buf;
  }
  if (ch->formatbuf) free(ch->formatbuf);
  ch->formatbuf = malloc(sizeof(char) * allocsize);
  strcpy(ch->formatbuf, source);
  
  return ch->formatbuf;
}

char * scanner_format(struct t_scanner *scanner) {
  char buf[SCRATCH_BUF_SIZE + 1];
  int len;
  char *toobig = "<#scanner: TOO_BIG>";
  int allocsize;
  char *source;
  struct t_char *current;
  struct t_char blank;
  
  if (scanner->current) {
    current = scanner->current;
  }
  else {
    memset(&blank, 0, sizeof(struct t_char));
    current = &blank;
  }
  
  len = snprintf(buf, SCRATCH_BUF_SIZE, "<#scanner: {c: %s, token: %s, row: %d, col: '%d', debug: %d, token_count: %d, stack_size: %d, error: %d}>",
     char_format(scanner->current),
     token_format(scanner->token),
     current->row,
     current->col,
     scanner->debug,
     scanner->token_count,
     scanner->stack_size,
     scanner->error);
  if (len > SCRATCH_BUF_SIZE) {
    allocsize = strlen(toobig) + 1;
    source = toobig;
  }
  else {
    allocsize = len + 1;
    source = buf;
  }
  if (scanner->formatbuf) free(scanner->formatbuf);
  scanner->formatbuf = malloc(sizeof(char) * allocsize);
  strcpy(scanner->formatbuf, buf);
  
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

/*
 * Get the next token.
 */
struct t_token * scanner_next(struct t_scanner *scanner) {
  struct t_token *token;
  int i;
  struct t_char *c;
  
  if (scanner->stack_size > 0) {
    token = scanner->token;
    for (i=1; i < scanner->stack_size; i++) {
      token = token->prev;
    }
    scanner->stack_size--;
    return token;
  }
  
  if (scanner_skip_whitespace(scanner)) return NULL;
  c = scanner_c(scanner);
  
  if (c->c_class == CC_EOF) {
    token = scanner_init_token(scanner, TT_EOF);
    scanner_token_char(scanner);
  }
  else if (c->c_class == CC_EOL) {
    token = scanner_parse_eol(scanner);
  }
  else if (c->c_class == CC_DIGIT) {
    token = scanner_parse_num(scanner);
  }
  else if (c->c_class == CC_ALPHA || c->c == '_') {
    token = scanner_parse_name(scanner);
  }
  else if (c->c_class == CC_OP) {
    token = scanner_parse_op(scanner);
  }
  else if (c->c_class == CC_DELIM) {
    token = scanner_parse_delim(scanner);
  }
  else {
    token = scanner_init_token(scanner, TT_UNKNOWN);
    scanner_token_char(scanner);
    if (!scanner_nextc(scanner)) return NULL;
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
  token->buf[0] = scanner_ch(scanner);
  token->buf[1] = '\0';
  
  return token->buf;
}

struct t_token * scanner_token(struct t_scanner *scanner) {
  if (!scanner->token) {
    return scanner_next(scanner);
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

struct t_token * scanner_parse_eol(struct t_scanner *scanner) {
  struct t_token *token;
  int size;
  int ch;
  char *str;
  
  ch = scanner_ch(scanner);
  token = scanner_init_token(scanner, TT_EOL);
  size = 2;
  if (ch == '\r') {
    ch = scanner_nextch(scanner);
    if (ch == '\n') {
      size++;
      str = "\r\n";
      scanner_nextch(scanner);
    }
    else {
      str = "\r";
      scanner_nextc(scanner);
    }
  }
  else if (ch == '\n') {
    str = "\n";
    scanner_nextc(scanner);
  }
  else {
    assert(0);
  }
  token->buf = malloc(sizeof(char) * size);
  strcpy(token->buf, str);  
  
  return token;
}

struct t_token * scanner_parse_num(struct t_scanner *scanner) {
  int i;
  struct t_token *token;
  char buf[SCRATCH_BUF_SIZE + 1];
  int buf_i = 0;
  struct t_char *c;
  
  c = scanner_c(scanner);
  token = scanner_init_token(scanner, TT_NUM);
  for (i=0; 1; ++i) {
    if (i >= MAX_NUM_LEN) {
      token->type = TT_ERROR;
      token->error = PERR_MAX_NUM_SIZE;
    }
    if (i < SCRATCH_BUF_SIZE) {
      buf[buf_i++] = c->c;
    }
    if ((c = scanner_nextc(scanner)) == NULL) {
      fprintf(stderr, "Failed on call to scanner_nextc()\n");
      return NULL;
    }
    if (c->c_class != CC_DIGIT) {
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
  struct t_char *c;
  int i = 0;
  
  c = scanner_c(scanner);
  token = scanner_init_token(scanner, TT_NAME);
  do {
    if (i == MAX_NAME_LEN) {
      token->type = TT_ERROR;
      token->error = PERR_MAX_NAME_SIZE;
    }
    if (i < SCRATCH_BUF_SIZE) {
      buf[buf_i++] = c->c;
    }
    if ((c = scanner_nextc(scanner)) == NULL) return NULL;
    i++;
  } while (c->c_class == CC_ALPHA || c->c_class == CC_DIGIT || c->c == '_');
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
  struct t_char *c;
  
  c = scanner_c(scanner);
  if (c->c == '+') {
    token = scanner_init_token(scanner, TT_PLUS);
  }
  else if (c->c == '=') {
    token = scanner_init_token(scanner, TT_EQUAL);
  }
  else {
    token = scanner_init_token(scanner, TT_UNKNOWN);
    return token;
  }
  int i;
  int invalid = 0;
  for (i=0; 1; i++) {
    buf[buf_i++] = c->c;
    if ((c = scanner_nextc(scanner)) == NULL) return NULL;
    if (c->c_class == CC_OP) {
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
  struct t_char *c;
  
  c = scanner_c(scanner);
  
  switch (c->c) {
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
  scanner_token_char(scanner);
  if (scanner_nextc(scanner) == NULL) return NULL;
  return token;
}

int scanner_skip_whitespace(struct t_scanner *scanner) {
  while (scanner_c(scanner)->c_class == CC_SPACE) {
    if (scanner_nextc(scanner) == NULL) return 1;
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
