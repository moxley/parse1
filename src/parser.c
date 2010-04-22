#include <assert.h>
#include <string.h>
#include "parser.h"

#define INDENT_BUF 80

/* Keywords */
char *parser_keywords[] = {
  "print",
  (char *) 0
};

char *parser_error_names[] = {
  "PARSER_ERR_NONE",
  "PARSER_ERR_MAX_ERRORS"
};

/* Expression types */
struct t_expr expression_types[] = {
  {EXP_NONE, "none", NULL, NULL, &parser_none_fmt, NULL, NULL},
  {EXP_ERROR, "error", NULL, NULL, NULL, NULL, NULL},
  {EXP_EOF, "eof", NULL, NULL, NULL, NULL, NULL},
  {EXP_NULL, "null", NULL, NULL, NULL, NULL, NULL},
  {EXP_NUM,  "num", &parser_num_init, NULL, &parser_num_fmt, &parser_num_parse, &parser_num_close},
  {EXP_FCALL, "fcall", &parser_fcall_init, NULL, &parser_fcall_fmt, &parser_fcall_parse, &parser_fcall_close},
  {EXP_TERM, "term", &parser_term_init, NULL, &parser_term_fmt, &parser_term_parse, &parser_term_close}
};
int expr_types_size = sizeof(expression_types);

char statement_buf[STATEMENT_FORMAT_BUF_SIZE];
char *nullexpr = "<#expr: {type: NULL}>";

struct t_expr noexpr;

int parser_init(struct t_parser *parser, FILE *in) {
  memset(parser, 0, sizeof(struct t_parser));
  noexpr.type = EXP_NONE;
  noexpr.format = &parser_none_fmt;
  parser->first = NULL;
  parser->stmt = &noexpr;
  parser->errors[0] = (struct t_parse_error *) 0;
  parser->error = PARSER_ERR_NONE;
  if (scanner_init(&(parser->scanner), in)) return 1;
  return 0;
}

int parser_count_errors(struct t_parser *parser) {
  int error_i = 0;
  struct t_token *token;
  
  do {
    if ((token = scanner_next(&(parser->scanner))) == NULL) return 1;
    printf("token: %s\n", token_format(token));
    if (token->type == TT_ERROR) {
      parser->errors[error_i] = malloc(sizeof(struct t_parse_error));
      if (error_i == MAX_PARSE_ERRORS) {
        scanner_init_token(&(parser->scanner), TT_ERROR);
        token_copy(&(parser->errors[MAX_PARSE_ERRORS]->token), token);
        parser->errors[MAX_PARSE_ERRORS]->token.error = PERR_MAX_ERRORS;
        break;
      }
      else {
        token_copy(&(parser->errors[error_i]->token), token);
      }
      error_i++;
    }
  } while(token->type != TT_EOF);
  exit(1);

  parser->errors[error_i] = (struct t_parse_error *) 0;
  return parser->error;
}

int parser_close(struct t_parser *parser) {
  int i;
  struct t_expr *stmt;
  struct t_expr *next;
  
  /* Free the scanner */
  scanner_close(&(parser->scanner));

  /* Free the parser errors */
  for (i=0; parser->errors[i]; i++) {
    free(parser->errors[i]);
  }

  /* Free the statements */
  stmt = parser->first;
  i = 0;
  while (stmt) {
    next = stmt->next;
    parser_expr_destroy(stmt);
    free(stmt);
    stmt = next;
  }

  return 0;
}

char * parser_format(struct t_parser *parser) {
  snprintf(parser->formatbuf, PARSER_FORMAT_BUF_SIZE, "<#parser: {error: %s, stmt: %s, scanner: %s}>",
    parse_error_names[parser->error],
    parser_expr_fmt(parser->stmt),
    scanner_format(&(parser->scanner)));
  return parser->formatbuf;
}

int parser_expr_init(struct t_expr *expr, int type) {
  struct t_expr *tpl;

  memset(expr, 0, sizeof(struct t_expr));
  tpl = &expression_types[type];
  expr->type = type;
  expr->name = tpl->name;
  expr->init = tpl->init;
  expr->copy = tpl->copy;
  expr->format = tpl->format;
  expr->parse = tpl->parse;
  expr->destroy = tpl->destroy;
  expr->next = NULL;
  expr->prev = NULL;
  
  if (expr->init) {
    if (expr->init(expr)) {
      return 1;
    }
  }
  
  return 0;
}

int parser_expr_destroy(struct t_expr *expr) {
  if (expression_types[expr->type].destroy) {
    expression_types[expr->type].destroy(expr);
  }
  if (expr->detail) {
    free(expr->detail);
    expr->detail = NULL;
  }
  if (expr->formatbuf) {
    free(expr->formatbuf);
    expr->formatbuf = NULL;
  }
  return 0;
}

/*
 * Add a statement to the parse tree.
 */
int parser_addstmt(struct t_parser *parser, struct t_expr *stmt) {
  stmt->next = NULL;
  if (!parser->first) {
    parser->first = stmt;
  }
  else {
    parser->stmt->next = stmt;
  }
  parser->stmt = stmt;
  
  return 0;
}

int parser_pushexpr(struct t_parser *parser, struct t_expr *expr) {
  assert(0); // Not implemented
  return 0;
}

struct t_expr * parser_popexpr(struct t_parser *parser) {
  assert(0); // Not implemented
  return NULL;
}

/*
 * Get next token
 */
struct t_token *parser_next(struct t_parser *parser) {
  return scanner_next(&parser->scanner);
}

/*
 * Get the current token.
 */
struct t_token *parser_token(struct t_parser *parser) {
  return scanner_token(&parser->scanner);
}

/*
 * Push back a token onto the token stream.
 */
void parser_pushtoken(struct t_parser *parser) {
  scanner_push(&parser->scanner);
}

struct t_token * parser_poptoken(struct t_parser *parser) {
  return scanner_pop(&parser->scanner);
}

/*
 * Convert an expression to a string for inspection.
 */
char * parser_expr_fmt(struct t_expr *expr) {
  char scratch_buf[PARSER_SCRATCH_BUF + 1];
  int len;
  char *message = "<#expr TOO_BIG>";
  char *detailbuf = NULL;
  
  if (!expr) {
    return nullexpr;
  }
  
  if (!expr->detail) {
    detailbuf = "NONE";
  }
  else {
    if (expr->format) {
      detailbuf = expr->format(expr);
    }
    else {
      detailbuf = "UNKNOWN";
    }
  }

  if (expr->formatbuf) free(expr->formatbuf);
  len = snprintf(scratch_buf, PARSER_SCRATCH_BUF, "<#expr: {type: %s, detail: %s}>", expr->name, detailbuf);
  if (len > PARSER_SCRATCH_BUF) {
    expr->formatbuf = malloc(sizeof(char) * (strlen(message) + 1));
    strcpy(expr->formatbuf, message);
  }
  else {
    expr->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(expr->formatbuf, scratch_buf);
  }

  return expr->formatbuf;
}

/*
 * Top-level parse.
 *
 * After calling this function, the parser will point to the
 * token immediately following the expression.
 */
struct t_expr * parser_parse(struct t_parser *parser) {
  struct t_expr *expr = NULL;
  struct t_token *token;

  token = parser_token(parser);
  if (token->type == TT_EOF) {
    fprintf(stderr, "%s(): Unexpected end of file\n", __FUNCTION__);
    return NULL;
  }

  expr = parser_parse_stmt(parser);
  if (!expr) return NULL;
  token = parser_token(parser);
  if (!token) return NULL;
  
  if (token->type != TT_EOL && token->type != TT_EOF) {
    fprintf(stderr, "Unexpected token. Expected EOL or EOF\n");
    do {
      token = parser_next(parser);
      if (!token) return NULL;
    } while (token->type != TT_EOL && token->type != TT_EOF);
  }
  
  while (token->type == TT_EOL) {
    token = parser_next(parser);
  }
  
  return expr;
}

struct t_expr * parser_parse_stmt(struct t_parser *parser) {
  struct t_token *token = NULL;
  struct t_expr *expr = NULL;
  
  token = parser_token(parser);
  if (token->type == TT_NAME) {
    token = parser_next(parser);
    parser_pushtoken(parser);
    if (token->type == TT_EQUAL) {
      parse_parse_assignment(parser);
    }
    else {
      expr = parser_parse_simple(parser);
    }
  }
  else {
    expr = parser_parse_simple(parser);
  }
  
  return expr;
}

struct t_expr * parse_parse_assignment(struct t_parser *parser) {
  return NULL;
}

struct t_expr * parser_parse_simple(struct t_parser *parser) {
  return parser_parse_term(parser);
}

struct t_expr * parser_parse_term(struct t_parser *parser) {
  struct t_token *token;
  struct t_expr *termexpr = NULL;
  struct t_binom *term;
  
  parser_parse_factor(parser);
  token = parser_token(parser);
  while (token->type == TT_PLUS || token->type == TT_PLUS) {
    termexpr = malloc(sizeof(struct t_expr));
    parser_expr_init(termexpr, EXP_TERM);
    term = (struct t_binom *) termexpr->detail;
    term->op = token;
    term->left = parser_popexpr(parser);
    
    parser_next(parser);
    
    parser_parse_factor(parser);
    term->right = parser_popexpr(parser);
    
    token = parser_token(parser);
  }
  
  return termexpr;
}

struct t_expr * parser_parse_factor(struct t_parser *parser) {
  struct t_token *token;
  struct t_expr *expr = NULL;
  
  token = parser_token(parser);
  if (token->type == TT_NUM) {
    // TODO Do number expression
    expr = parser_num_parse(parser);
  }
  else if (token->type == TT_NAME) {
    token = parser_next(parser);
    if (token->type == TT_PARENL) {
      parser_pushtoken(parser);
      expr = parser_fcall_parse(parser);
    }
    else if (token->type == TT_EQUAL) {
      parser_pushtoken(parser);
      expr = parser_term_parse(parser);
    }
    else {
      fprintf(stderr, "%s(): Unknown expression: TT_NAME, %s\n", __FUNCTION__, token_types[token->type]);
      while (token->type != TT_EOL && token->type != TT_EOF) {
        token = parser_next(parser);
        if (token->type == TT_EOL) {
          token = parser_next(parser);
        }
      }
      return NULL;
    }
  }
  else {
    expr = &noexpr;
    parser_next(parser);
  }
  
  return expr;
}

/*
 * Copy an expression.
 */
int expr_copy(struct t_expr *dest, struct t_expr *source) {
  dest->type = source->type;
  dest->name = source->name;
  dest->init = source->init;
  dest->format = source->format;
  dest->parse = source->parse;
  dest->destroy = source->destroy;
  dest->copy = source->copy;
  dest->next = NULL;
  dest->detail = NULL;
  if (dest->copy) dest->copy(dest, source);
  dest->formatbuf = NULL;
  return 0;
}

char * parser_none_fmt(struct t_expr *expr) {
  assert(EXP_NONE == expr->type);
  snprintf(expr->formatbuf, STATEMENT_FORMAT_BUF_SIZE, "<#expr: {type: none}>");
  return expr->formatbuf;
}

/*
 * Initialize a function call
 */
int parser_fcall_init(struct t_expr *expr) {
  struct t_fcall *call;
  
  call = calloc(1, sizeof(struct t_fcall));
  call->name = NULL;
  call->firstarg = NULL;
  call->argcount = 0;
  call->formatbuf = NULL;
  expr->detail = (void *) call;

  return 0;
}

/*
 * Parse a function call
 */
struct t_expr * parser_fcall_parse(struct t_parser *parser) {
  struct t_token *token;
  struct t_fcall *call;
  struct t_expr *expr = NULL;
  struct t_expr *argexpr = NULL;
  struct t_expr *prev = NULL;

  expr = malloc(sizeof(struct t_expr));
  parser_expr_init(expr, EXP_FCALL);
  call = (struct t_fcall *) expr->detail;

  token = parser_token(parser);
  if (!token) return NULL;
  if (token->type != TT_NAME) {
      // TODO return ERROR expression
      fprintf(stderr, "(TODO) Syntax error. Expected function name. Got %s: '%s'\n", token_types[token->type], token->buf);
      return NULL;
  }
  if (call->name) free(call->name);
  call->name = malloc(sizeof(char) * (strlen(token->buf) + 1));
  strcpy(call->name, token->buf);

  token = parser_next(parser);
  if (!token) return NULL;
  if (token->type != TT_PARENL) {
    fprintf(stderr, "(TODO) %s(): Syntax error: Expected function opening parenthese '('. Got %s: %s\n", __FUNCTION__, token_types[token->type], token->buf);
    return NULL;
  }

  token = parser_next(parser);
  if (!token) return NULL;
  if (token->type == TT_PARENR) {
    token = parser_next(parser);
  }
  else {
    while (1) {
      if (token->type == TT_EOF) {
        fprintf(stderr, "(TODO) (in call to %s) Unexpected end of file", call->name);
        return NULL;
      }
      argexpr = parser_parse_simple(parser);
      if (!call->firstarg) {
        call->firstarg = argexpr;
      }
      else {
        prev->next = argexpr;
      }
      call->argcount++;
      prev = argexpr;

      token = parser_token(parser);
      if (!token) return NULL;
      if (token->type == TT_PARENR) {
        parser_next(parser);
        break;
      }
      else if (token->type != TT_COMMA) {
        fprintf(stderr, "(TODO) (in call to %s) Syntax error: Expected function argument or closing parenthese ')'. Got: %s\n", call->name, token_format(token));
        return NULL;
      }
      else {
        parser_next(parser);
      }
    }
  }

  return expr;
}

/*
 * Format a function call.
 */
char * parser_fcall_fmt(struct t_expr *expr) {
  char scratch_buf[PARSER_SCRATCH_BUF + 1];
  char args_buf[PARSER_SCRATCH_BUF + 1];
  int len;
  struct t_fcall *call;
  char *message = "<#fcall TOO_BIG>";
  struct t_expr *arg;
  char *emptyargs = "[]";
  char *a;

  assert(EXP_FCALL == expr->type);
  call = (struct t_fcall *) expr->detail;
  
  /*
   * Format the arguments
   */
  if (!call->firstarg) {
    strcpy(args_buf, emptyargs);
  }
  else {
    len = strlen(emptyargs);
    arg = call->firstarg;
    while (arg) {
      if (arg != call->firstarg) {
        len += 2; // Lenth of ", "
      }
      a = parser_expr_fmt(arg);
      len += strlen(a);
      arg = arg->next;
    }
    
    if (len > PARSER_SCRATCH_BUF) {
      strcpy(args_buf, "TOO_BIG");
    }
    else {
      strcpy(args_buf, "[");
      arg = call->firstarg;
      while (arg) {
        if (arg != call->firstarg) {
          strcat(args_buf, ", ");
        }
        strcat(args_buf, arg->formatbuf);
        arg = arg->next;
      }
      strcat(args_buf, "]");
    }
  }

  len = snprintf(scratch_buf, PARSER_SCRATCH_BUF, "<#fcall: {name: %s, args: %s}>",
    call->name,
    args_buf);

  if (call->formatbuf) {
    free(call->formatbuf);
  }
  if (len > PARSER_SCRATCH_BUF) {
    call->formatbuf = malloc(sizeof(char) * (strlen(message) + 1));
    strcpy(call->formatbuf, message);
  }
  else {
    call->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(call->formatbuf, scratch_buf);
  }

  return call->formatbuf;
}

int parser_fcall_close(struct t_expr *expr) {
  struct t_fcall *call;
  struct t_expr *arg;
  struct t_expr *next;
  
  call = (struct t_fcall *) expr->detail;
  arg = call->firstarg;
  while (arg) {
    next = arg->next;
    free(arg);
    arg = next;
  }
  
  if (call->formatbuf) free(call->formatbuf);
  if (call->name) free(call->name);
  
  return 0;
}

int parser_num_init(struct t_expr *expr) {
  struct t_expr_num *num;
  
  num = calloc(1, sizeof(struct t_expr_num));
  expr->detail = (void *) num;
  
  return 0;
}

char * parser_num_fmt(struct t_expr *expr) {
  struct t_expr_num *num;
  char scratch[PARSER_SCRATCH_BUF + 1];
  int len;
  char *toobig = "TOO_BIG";
  
  num = (struct t_expr_num *) expr->detail;
  len = snprintf(scratch, PARSER_SCRATCH_BUF, "<#num {type=%d, value=%d}>", num->type, num->value);
  if (num->formatbuf) free(num->formatbuf);
  if (len > PARSER_SCRATCH_BUF) {
    num->formatbuf = malloc(sizeof(char) * (strlen(toobig) + 1));
    strcpy(num->formatbuf, toobig);
  }
  else {
    num->formatbuf = malloc(sizeof(char) * (strlen(scratch) + 1));
    strcpy(num->formatbuf, scratch);
  }
  
  return num->formatbuf;
}

struct t_expr * parser_num_parse(struct t_parser *parser) {
  struct t_token *token;
  struct t_expr_num *num;
  struct t_expr *expr;
  
  // Get the token
  token = parser_token(parser);
  if (token->type != TT_NUM) {
    fprintf(stderr, "(TODO) Expected number. Got: %s\n", token_format(token));
    return NULL;
  }

  // Create the expression
  expr = malloc(sizeof(struct t_expr));
  parser_expr_init(expr, EXP_NUM);
  
  // Get the numeric value
  num = (struct t_expr_num *) expr->detail;
  num->value = atoi(token->buf);
  
  parser_next(parser);
  
  return expr;
}

int parser_num_close(struct t_expr *expr) {
  struct t_expr_num *num;

  if (expr->detail) {
    num = (struct t_expr_num *) expr->detail;
    free(num->formatbuf);
  }

  return 0;
}

int parser_binom_init(struct t_expr *expr) {
  struct t_binom *binom;

  binom = malloc(sizeof(struct t_binom));
  binom->sign = NULL;
  binom->left = NULL;
  binom->right = NULL;
  binom->op = NULL;
  expr->detail = (void *) binom;

  return 0;
}

char * parser_binom_fmt(struct t_expr *expr) {
  char buf[SCRATCH_BUF_SIZE+1];
  int len;
  struct t_binom *binom;
  char *toobig = "<#binom {TOO_BIG}>";

  assert(expr->detail);
  binom = (struct t_binom *) expr->detail;
  len = snprintf(buf, SCRATCH_BUF_SIZE, "<#binom {op: %s, left: %s, right: %s}>",
      token_format(binom->op),
      parser_expr_fmt(binom->left),
      parser_expr_fmt(binom->right));
  if (binom->formatbuf) free(binom->formatbuf);
  if (len > SCRATCH_BUF_SIZE) {
    binom->formatbuf = malloc(sizeof(char) * (strlen(toobig) + 1));
    strcpy(binom->formatbuf, toobig);
  }
  else {
    binom->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(binom->formatbuf, buf);
  }

  return binom->formatbuf;
}

struct t_expr * parser_binom_parse(struct t_parser *parser) {
  //struct t_binom *binom;
  return NULL;
}

int parser_binom_close(struct t_expr *expr) {
  struct t_binom *binom;

  if (!expr->detail) return 0;
  binom = expr->detail;

  /* op, left and right should be deallocated by other functions */

  if (binom->formatbuf) free(binom->formatbuf);

  return 0;
}

int parser_term_init(struct t_expr *expr) {
  return parser_binom_init(expr);
}

char * parser_term_fmt(struct t_expr *expr) {
  return parser_binom_fmt(expr);
}

struct t_expr * parser_term_parse(struct t_parser *parser) {
  return parser_binom_parse(parser);
}

int parser_term_close(struct t_expr *expr) {
  return parser_binom_close(expr);
}
