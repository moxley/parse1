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
  "NONE",
  "MAX_TOKEN_SIZE",
  "MAX_NUM_SIZE",
  "MAX_NAME_SIZE",
  "MAX_ERRORS",
  "MAX_STRING_SIZE",
  "UNEXPECTED_EOF"
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
  "TT_COMMA",
  "TT_SEMI",
  "TT_STRING",
  "TT_LT",
  "TT_GT"
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

char *scanner_operators = "~!@%^&*-+=|?/<>";
char *scanner_delimiters = "()[]{}.:,;";
char *scanner_quotes = "\"'`";

/*
 * Initialize a scanner.
 */
int scanner_init(struct t_scanner *scanner, FILE *in)
{
  memset(scanner, 0, sizeof(struct t_scanner));
  scanner->in = in;
  
  if (!scanner_cc_table_initialized) {
    scanner_build_cc_table();
    scanner_cc_table_initialized = 1;
  }

  scanner_init_token(scanner, &scanner->unknown, TT_UNKNOWN);

  list_init(&scanner->t_list);
  list_init(&scanner->t_pushback);
  list_init(&scanner->c_list);
  list_init(&scanner->c_pushback);

  return 0;
}

/**
 * Deallocate or close attached resources from a scanner.
 */
void scanner_close(struct t_scanner *scanner) {
  struct t_token *t;
  struct t_char *c;
  struct item *item;

  DBG(2, "Begin.");

  fclose(scanner->in);

  item = scanner->c_list.first;
  while (item) {
    c = (struct t_char *) item->value;
    if (c->formatbuf) free(c->formatbuf);
    free(c);
    item = item->next;
  }

  item = scanner->t_list.first;
  while (item) {
    t = (struct t_token *) item->value;
    if (t->buf) free(t->buf);
    if (t->formatbuf) free(t->formatbuf);
    item = item->next;
  }

  list_empty(&scanner->c_list);
  list_empty(&scanner->c_pushback);
  list_empty(&scanner->t_list);
  list_empty(&scanner->t_pushback);

  DBG(3, "End.");
}

/**
 * Allocate and initialize a token.
 */
struct t_token * scanner_create_token(struct t_scanner *scanner, int type)
{
  struct t_token *token;
  
  token = malloc(sizeof(struct t_token));
  scanner_init_token(scanner, token, type);
  
  list_push(&scanner->t_list, token);
  scanner->token = token;
  
  return scanner->token;
}

/**
 * Initialize a token.
 */
void scanner_init_token(struct t_scanner *scanner, struct t_token *token, int type)
{
  struct t_char *c;
  
  c = scanner_c(scanner);
  memset(token, 0, sizeof(struct t_token));
  
  token->type = type;
  token->buf = NULL;
  token->error = 0;
  token->row = c->row;
  token->col = c->col;
  token->formatbuf = NULL;
}

void token_copy(struct t_token *dest, const struct t_token *source)
{
  dest->type = source->type;
  if (dest->buf) free(dest->buf);
  if (!source->buf) {
	dest->buf = NULL;
  }
  else {
    dest->buf = malloc(sizeof(char) * (strlen(source->buf) + 1));
    strcpy(dest->buf, source->buf);
  }
  dest->error = source->error;
  dest->row = source->row;
  dest->col = source->col;
  if (dest->formatbuf) free(dest->formatbuf);
  if (!source->formatbuf) {
	dest->formatbuf = NULL;
  }
  else {
    dest->formatbuf = malloc(sizeof(char) * (strlen(source->formatbuf) + 1));
    strcpy(dest->formatbuf, source->formatbuf);
  }
}

int scanner_nextch(struct t_scanner *scanner) {
  return scanner_nextc(scanner)->c;
}

struct t_char * scanner_nextc(struct t_scanner *scanner) {
  struct t_char *c;
  
  if (list_size(&scanner->c_pushback) > 0) {
    scanner->current = (struct t_char *) list_pop(&scanner->c_pushback);
  }
  else {
    c = malloc(sizeof(struct t_char));
    memset(c, 0, sizeof(struct t_char));
    c->c = getc(scanner->in);
    if (!scanner->current) {
      c->col = 0;
      c->row = 0;
    }
    else if ((scanner->current->c == '\r' && c->c != '\n') || scanner->current->c == '\n') {
      c->col = 0;
      c->row = scanner->current->row + 1;
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
  list_push(&scanner->c_pushback, scanner->current);
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
  
  len = snprintf(buf, SCRATCH_BUF_SIZE, "<#scanner: {c: %s, token: %s, debug: %d, token_count: %d, pushback_size: %d, error: %d}>",
     char_format(scanner->current),
     token_format(scanner->token),
     scanner->debug,
     list_size(&scanner->t_list),
     list_size(&scanner->t_pushback),
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
  
  if (token->buf) {
    util_escape_string(esc_buf, SCRATCH_BUF_SIZE, token->buf);
  }
  else {
    strcpy(esc_buf, "");
  }
  len = snprintf(buf,
           SCRATCH_BUF_SIZE,
           "<#token {type: %s, error: %s, row: %d, col: %d, buf: '%s'}>",
           token_types[token->type],
           parse_error_names[token->error],
           token->row,
           token->col,
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
  struct t_char *c;
  
  if (list_size(&scanner->t_pushback) > 0) {
    token = (struct t_token *) list_pop(&scanner->t_pushback);
    return token;
  }
  
  c = scanner_c(scanner);
  if (scanner_skip_whitespace(scanner)) return NULL;
  c = scanner_c(scanner);
  
  if (c->c_class == CC_EOF) {
    token = scanner_create_token(scanner, TT_EOF);
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
  else if (c->c_class == CC_QUOTE) {
    token = scanner_parse_string(scanner);
  }
  else {
    token = scanner_create_token(scanner, TT_UNKNOWN);
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
 * Push the token into the pushback stack.
 */
void scanner_push(struct t_scanner *scanner) {
  list_push(&scanner->t_pushback, scanner->token);
}

/*
 * Pop a token from the pushback stack.
 */
struct t_token * scanner_pop(struct t_scanner *scanner) {
  scanner->token = list_pop(&scanner->t_pushback);
  return scanner->token;
}

struct t_token * scanner_parse_eol(struct t_scanner *scanner) {
  struct t_token *token;
  int size;
  int ch;
  char *str;
  
  ch = scanner_ch(scanner);
  token = scanner_create_token(scanner, TT_EOL);
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
  token = scanner_create_token(scanner, TT_NUM);
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
  token = scanner_create_token(scanner, TT_NAME);
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

struct t_token * scanner_parse_op(struct t_scanner *scanner)
{
  struct t_token *token;
  char buf[SCRATCH_BUF_SIZE + 1];
  int buf_i = 0;
  struct t_char *c;
  int i;
  
  c = scanner_c(scanner);
  if (c->c == '+') {
    token = scanner_create_token(scanner, TT_PLUS);
  }
  else if (c->c == '=') {
    token = scanner_create_token(scanner, TT_EQUAL);
  }
  else if (c->c == '<') {
    token = scanner_create_token(scanner, TT_LT);
  }
  else if (c->c == '>') {
    token = scanner_create_token(scanner, TT_GT);
  }
  else {
    token = scanner_create_token(scanner, TT_UNKNOWN);
  }
  for (i=0; c->c_class == CC_OP && i<SCRATCH_BUF_SIZE; i++) {
    buf[buf_i++] = c->c;
    if ((c = scanner_nextc(scanner)) == NULL) return NULL;
  }
  
  buf[buf_i] = '\0';
  if (token->buf) free(token->buf);
  token->buf = malloc(sizeof(char) * (buf_i + 1));
  strcpy(token->buf, buf);

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
  case ';':
    type = TT_SEMI;
    break;
  }

  token = scanner_create_token(scanner, type);
  scanner_token_char(scanner);
  if (scanner_nextc(scanner) == NULL) return NULL;
  return token;
}

struct t_token * scanner_parse_string(struct t_scanner *scanner)
{
  char quotechar;
  struct t_char *c;
  int i;
  int len = 0;
  char buf[MAX_STRING_LEN+1];
  struct t_token *token;
  int append = 1;
  int token_type = TT_STRING;
  int token_error = PERR_NONE;
  
  quotechar = scanner_ch(scanner);
  i = 0;
  while (1) {
    c = scanner_nextc(scanner);
    if (i >= MAX_STRING_LEN) {
      append = 0;
    }
    
    if (c->c_class == CC_EOF) {
      token_type = TT_ERROR;
      token_error = PERR_UNEXPECTED_EOF;
      break;
    }
    else if (c->c_class == CC_QUOTE && c->c == quotechar) {
      c = scanner_nextc(scanner);
      // We're done
      break;
    }
    else if (c->c == '\\') {
      // Escape sequence
      c = scanner_nextc(scanner);
      if (c->c_class == CC_EOF) {
        token_type = TT_ERROR;
        token_error = PERR_UNEXPECTED_EOF;
        break;
      }
      if (c->c == 'n') {
        if (append) buf[len++] = '\n';
        i++;
      }
      else if (c->c == 'r') {
        if (append) buf[len++] = '\r';
        i++;
      }
      else if (c->c == 'b') {
        if (append) buf[len++] = '\b';
        i++;
      }
      else {
        if (append) buf[len++] = c->c;
        i++;
      }
    }
    else {
      if (append) buf[len++] = c->c;
      i++;
    }
  }
  
  if (i >= MAX_STRING_LEN) {
    token_type = TT_ERROR;
    token_error = PERR_MAX_STRING_SIZE;
    buf[MAX_STRING_LEN] = '\0';
  }
  else {
    buf[i] = '\0';
  }

  // Create the token
  token = scanner_create_token(scanner, token_type);
  token->error= token_error;

  // Copy buf to token
  token->buf = malloc(sizeof(char) * (len + 1));
  strcpy(token->buf, buf);
  
  return token;
}

int scanner_skip_whitespace(struct t_scanner *scanner)
{
  while (scanner_c(scanner)->c_class == CC_SPACE) {
    if (scanner_nextc(scanner) == NULL) return 1;
  }
  return 0;
}

int scanner_charclass(int c)
{
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

void scanner_build_cc_table()
{
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
int util_escape_string(char buf[], int buf_size, const char *str)
{
  int i;
  int buf_i = 0;
  int est_i = 0;
  char esc_buf[3];
  int esc_len;
  
  for (i=0; str[i]; ++i) {
    esc_len = util_escape_char(esc_buf, str[i]);
    if (buf_i + esc_len < buf_size-1) {
      strcpy(&buf[buf_i], esc_buf);
      buf_i += esc_len;
    }
    est_i += esc_len;
  }
  buf[buf_i] = '\0';
  return est_i;
}

int util_escape_char(char *buf, char c)
{
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
