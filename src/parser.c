#include <assert.h>
#include <string.h>
#include "parser.h"

#define INDENT_BUF 80
#define PARSER_SCRATCH_BUF 1024

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
  {EXP_NONE, "none", NULL, &parser_none_fmt, NULL, NULL},
  {EXP_NONE, "error", NULL, NULL, NULL, NULL},
  {EXP_NONE, "eof", NULL, NULL, NULL, NULL},
  {EXP_FCALL, "fcall", &parser_fcall_init, &parser_fcall_fmt, &parser_fcall_parse, &parser_fcall_close}
};
int expr_types_size = sizeof(expression_types);

char statement_buf[STATEMENT_FORMAT_BUF_SIZE];

struct t_expr noexpr;

int parser_init(struct t_parser *parser, FILE *in) {
  noexpr.type = EXP_NONE;
  noexpr.format = &parser_none_fmt;
  parser->first = NULL;
  parser->expr = &noexpr;
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

  parser->errors[error_i] = (struct t_parse_error *) 0;
  return parser->error;
}

int parser_close(struct t_parser *parser) {
  int i;
  struct t_expr *expr;
  struct t_expr *next;

  /* Free the scanner */
  scanner_close(&(parser->scanner));

  /* Free the parser errors */
  for (i=0; parser->errors[i]; i++) {
    free(parser->errors[i]);
  }

  /* Free the expressions */
  expr = parser->first;
  i = 0;
  while (expr) {
    next = expr->next;
    parser_expr_destroy(expr);
    free(expr);
    expr = next;
  }

  return 0;
}

char * parser_format(struct t_parser *parser) {
  snprintf(parser->formatbuf, PARSER_FORMAT_BUF_SIZE, "<#parser: {error: %s, expr: %s, scanner: %s}>",
    parse_error_names[parser->error],
    parser_expr_fmt(parser->expr),
    scanner_format(&(parser->scanner)));
  return parser->formatbuf;
}

int parser_expr_destroy(struct t_expr *expr) {
  if (expression_types[expr->type].destroy) {
    expression_types[expr->type].destroy(expr);
  }
  if (expr->detail) free(expr->detail);
  if (expr->formatbuf) free(expr->formatbuf);
  return 0;
}

int parser_expr_init(struct t_expr *expr, int type) {
  struct t_expr *tpl;
  
  tpl = &expression_types[type];
  expr->type = type;
  expr->name = tpl->name;
  expr->init = tpl->init;
  expr->format = tpl->format;
  expr->parse = tpl->parse;
  expr->destroy = tpl->destroy;
  expr->next = NULL;
  expr->formatbuf = NULL;
  
  if (expr->init) {
    if (expr->init(expr)) {
      return 1;
    }
  }
  
  return 0;
}

/*
 * Add an expression to the parse tree.
 */
int parser_addexpr(struct t_parser *parser, struct t_expr *expr) {
  expr->next = NULL;
  if (!parser->first) {
    parser->first = expr;
  }
  else {
    parser->expr->next = expr;
  }
  parser->expr = expr;
  return 0;
}

struct t_expr * parser_expr(struct t_parser *parser) {
  return parser->expr;
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
  
  call = malloc(sizeof(struct t_fcall));
  call->name = NULL;
  call->argcount = 0;
  call->firstarg = NULL;
  expr->detail = (void *) call;

  return 0;
}

char * parser_expr_fmt(struct t_expr *expr) {
  char scratch_buf[PARSER_SCRATCH_BUF + 1];
  int len;
  char *message = "<#expr TOO_BIG>";
  char *detailbuf = NULL;
  
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
 * Format a function call.
 */
char * parser_fcall_fmt(struct t_expr *expr) {
  char scratch_buf[PARSER_SCRATCH_BUF + 1];
  int len;
  struct t_fcall *call;
  char *message = "<#fcall TOO_BIG>";

  assert(EXP_FCALL == expr->type);
  call = (struct t_fcall *) expr->detail;
  if (call->formatbuf) free(call->formatbuf);
  len = snprintf(scratch_buf, PARSER_SCRATCH_BUF, "<#fcall: {name: %s, args: %d}>",
    call->name,
    0);
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
  parser_addexpr(parser, expr);
  call = (struct t_fcall *) expr->detail;

  token = parser_next(parser);
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
    fprintf(stderr, "(TODO) Syntax error: Expected function opening parenthese '('. Got %s: %s\n", token_types[token->type], token->buf);
    return NULL;
  }

  while (1) {
    token = parser_next(parser);
    if (!token) return NULL;
    if (token->type == TT_PARENR) {
      break;
    }
    else if (token->type == TT_EOF) {
      fprintf(stderr, "(TODO) Syntax error: Expected function arguments or closing parenthese ')'. Got EOF.\n");
      return NULL;
    }
    else {
      parser_pushtoken(parser);
      argexpr = parser_expr_parse(parser);
      if (!call->firstarg) {
        call->firstarg = argexpr;
      }
      else {
        prev->next = argexpr;
      }
      call->argcount++;
      prev = argexpr;
    }

    token = parser_next(parser);
    if (!token) return NULL;
    if (token->type == TT_PARENR) {
      parser_pushtoken(parser);
    }
    else if (token->type != TT_COMMA) {
      fprintf(stderr, "(TODO) Syntax error: Expected function argument or closing parenthese ')'\n");
      return NULL;
    }
  }

  return expr;
}

struct t_token *parser_next(struct t_parser *parser) {
  return scanner_next(&parser->scanner);
}

struct t_token *parser_token(struct t_parser *parser) {
  return scanner_token(&parser->scanner);
}

/*
 * Push back a token onto the token stream.
 */
void parser_pushtoken(struct t_parser *parser) {
  scanner_push(&parser->scanner);
}

/*
 * Parse an expression.
 */
struct t_expr * parser_expr_parse(struct t_parser *parser) {
  struct t_token *token;
  
  token = parser_next(parser);
  if (token->type == TT_EOF) {
    // TODO Do EOF expression
  }
  else if (token->type == TT_NUM) {
    // TODO Do number expression
  }
  
  return NULL;
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
  }
  
  if (call->formatbuf) free(call->formatbuf);
  if (call->name) free(call->name);
  
  return 0;
}
