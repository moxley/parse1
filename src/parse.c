#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parse.h"

char scratch_buf[SCRATCH_BUF_SIZE];

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
  "TT_EOF",
  "TT_NUM",
  "TT_NAME",
  "TT_EQUAL",
  "TT_PLUS",
};

char * scanner_cc_names[] = {
  "CC_UNKNOWN",
  "CC_EOF",
  "CC_SPACE",
  "CC_DIGIT",
  "CC_ALPHA",
  "CC_OP",
  "CC_DELIM",
  "CC_QUOTE"
};

/* Keywords */
char *parser_keywords[] = {
  "print",
  (char *) 0
};

char *scanner_operators = "~!@%^&*-+=|?/";
char *scanner_delimiters = "()[]{}.:";
char *scanner_quotes = "\"'`";

char *parser_error_names[] = {
  "PARSER_ERR_NONE",
  "PARSER_ERR_MAX_ERRORS"
};

int scanner_init(struct t_scanner *scanner, FILE *in) {
  scanner->c = '\0';
  scanner->c_class = CC_UNKNOWN;
  scanner->reuse = 0;
  scanner->error = ERR_NONE;
  scanner->in = in;

  if (!scanner_cc_table_initialized) {
    scanner_build_cc_table();
    scanner_cc_table_initialized = 1;
  }

  if (scanner_getc(scanner)) {
    scanner->error = ERR_READ;
    return 1;
  }

  token_init(&(scanner->token), TT_UNKNOWN);

  return 0;
}

void token_init(struct t_token *token, int type) {
  token->buf_i = 0;
  token->buf[0] = '\0';
  token->type = type;
  token->error = PERR_NONE;
  token->format[0] = '\0';
}

void token_copy(struct t_token *dest, const struct t_token *source) {
  dest->buf_i = source->buf_i;
  strcpy(dest->buf, source->buf);
  dest->type = source->type;
  dest->error = source->error;
  strcpy(dest->format, source->format);
}

void scanner_close(struct t_scanner *scanner) {
  fclose(scanner->in);
}

int scanner_getc(struct t_scanner *scanner) {
  char esc_char[3];
  
  if (scanner->reuse) {
    scanner->reuse = 0;
  }
  else {
    scanner->c = getc(scanner->in);
  }
  scanner->c_class = scanner_charclass(scanner->c);

  /*
  util_escape_char(esc_char, scanner->c);
  printf("scanner_getc() c: '%s', c_class: %s\n",
	 esc_char,
	 scanner_cc_names[scanner->c_class]);
  */
  
  return 0;
}

void scanner_print(struct t_scanner *scanner) {
  scanner_format(scanner);
  printf(scanner->format);
}

int scanner_format(struct t_scanner *scanner) {
  char esc_char[3];
  token_format(&(scanner->token));
  util_escape_char(esc_char, scanner->c);
  snprintf(scanner->format, SCANNER_FORMAT_BUF_SIZE, "<#scanner: {c: '%s', c_class: %s, token: %s}>\n",
	 esc_char,
	 scanner_cc_names[scanner->c_class],
	 scanner->token.format);
}

int token_format(struct t_token *token) {
  int n;
  util_escape_string(scratch_buf, SCRATCH_BUF_SIZE, token->buf);
  n = snprintf(token->format,
	       TOKEN_FORMAT_BUF_SIZE,
	       "<#token {type: %s, error: %s buf: '%s'}>",
	       token_types[token->type],
	       parse_error_names[token->error],
	       scratch_buf);
  return n > TOKEN_FORMAT_BUF_SIZE;
}

int scanner_token(struct t_scanner *scanner) {
  if (scanner_skip_whitespace(scanner)) return 1;

  if (scanner->c_class == CC_EOF) {
    scanner->token.type = TT_EOF;
    scanner->token.buf[0] = '\0';
  }
  else if (scanner->c_class == CC_DIGIT) {
    scanner_token_num(scanner);
  }
  else if (scanner->c_class == CC_ALPHA) {
    scanner_token_name(scanner);
  }
  else if (scanner->c_class == CC_OP) {
    scanner_token_op(scanner);
  }
  else {
    scanner->token.type = TT_UNKNOWN;
    scanner->token.buf[0] = scanner->c;
    scanner->token.buf[1] = '\0';
  }
  return 0;
}

int scanner_token_num(struct t_scanner *scanner) {
  int i;
  token_init(&(scanner->token), TT_NUM);
  for (i=0; 1; ++i) {
    if (i >= MAX_NUM_LEN) {
      scanner->token.type = TT_ERROR;
      scanner->token.error = PERR_MAX_NUM_SIZE;
    }
    if (token_append(scanner)) return 1;
    if (scanner_getc(scanner)) return 1;
    if (scanner->c_class != CC_DIGIT) {
      break;
    }
  }
  return 0;
}

int scanner_token_name(struct t_scanner *scanner) {
  token_init(&(scanner->token), TT_NAME);
  while (1) {
    if (token_append(scanner)) return 1;
    if (scanner_getc(scanner)) return 1;
    if (!isalpha(scanner->c)) {
      break;
    }
  }
  return 0;
}

int scanner_token_op(struct t_scanner *scanner) {
  if (scanner->c == '+') {
    token_init(&(scanner->token), TT_PLUS);
  }
  else if (scanner->c == '=') {
    token_init(&(scanner->token), TT_EQUAL);
  }
  else {
    token_init(&(scanner->token), TT_UNKNOWN);
    return 0;
  }
  int i;
  int invalid = 0;
  for (i=0; 1; i++) {
    if (token_append(scanner)) return 1;
    if (scanner_getc(scanner)) return 1;
    if (scanner->c_class == CC_OP) {
      invalid = 1;
    }
    else {
      break;
    }
  }
  if (invalid) {
    scanner->token.type = TT_UNKNOWN;
  }
  return 0;
}

int token_append(struct t_scanner *scanner)
{
  if (scanner->token.buf_i >= TOKEN_BUF_SIZE) {
    scanner->error = PERR_MAX_TOKEN_SIZE;
    return 1;
  }
  scanner->token.buf[scanner->token.buf_i] = scanner->c;
  scanner->token.buf[scanner->token.buf_i+1] = '\0';
  scanner->token.buf_i++;
  return 0;
}

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

int parser_init(struct t_parser *parser, FILE *in) {
  parser->errors[0] = (struct t_parse_error *) 0;
  parser->error = PARSER_ERR_NONE;
  if (scanner_init(&(parser->scanner), in)) return 1;
  return 0;
}

int parser_count_errors(struct t_parser *parser) {
  int error_i = 0;
  struct t_parse_error max_errors_error;
  
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
