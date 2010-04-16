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

char statement_buf[STATEMENT_FORMAT_BUF_SIZE];

struct t_expr noexpr;

int parser_init(struct t_parser *parser, FILE *in) {
  noexpr.type = EXP_NONE;
  noexpr.fmt = &parser_none_fmt;
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
    free(expr);
    expr = next;
  }

  return 0;
}

int parser_format(struct t_parser *parser) {
  scanner_format(&(parser->scanner));
  parser->expr->fmt(parser->expr);
  snprintf(parser->format, PARSER_FORMAT_BUF_SIZE, "<#parser: {error: %s, expr: %s, scanner: %s}>",
    parse_error_names[parser->error],
    parser->expr->format,
    parser->scanner.format);
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
  printf("Setting parser->expr to type=%d\n", expr->type);
  parser->expr = expr;
  return 0;
}

char * parser_none_fmt(struct t_expr *expr) {
  assert(EXP_NONE == expr->type);
  snprintf(expr->format, STATEMENT_FORMAT_BUF_SIZE, "<#expr: {type: none}>");
  return expr->format;
}

/*
 * Initialize a function call
 */
int parser_fcall_init(struct t_fcall *call) {
  call->type = EXP_FCALL;
  call->fmt = &parser_fcall_fmt;
  return 0;
}

/*
 * Format a function call.
 */
char * parser_fcall_fmt(struct t_expr *expr) {
  struct t_fcall *call;

  assert(EXP_FCALL == expr->type);
  call = (struct t_fcall *) expr;
  snprintf(expr->format, STATEMENT_FORMAT_BUF_SIZE, "<#expr: {type: function, name: %s, args: %d}>",
    call->name,
    0);
  return expr->format;
}

/*
 * Parse a function call
 */
struct t_fcall *parser_fcall_parse(struct t_parser *parser) {
  struct t_token *token;
  struct t_fcall *call;

  parser_format(parser);

  token = parser_next(parser);
  if (!token) return NULL;
  if (token->type != TT_NAME) {
      // TODO return ERROR expression
      fprintf(stderr, "(TODO) Syntax error. Expected function name. Got %s: '%s'\n", token_types[token->type], token->buf);
      return NULL;
  }
  call = malloc(sizeof(struct t_fcall));
  if (parser_fcall_init(call)) return NULL;
  strcpy(call->name, token->buf);
  parser_addexpr(parser, (struct t_expr *) call);

  token = parser_next(parser);
  if (!token) return NULL;
  if (token->type != TT_PARENL) {
    fprintf(stderr, "(TODO) Syntax error: Expected function opening parenthese '('. Got %s: %s\n", token_types[token->type], token->buf);
    return NULL;
  }

  token = parser_next(parser);
  if (!token) return NULL;
  if (token->type != TT_PARENR) {
    fprintf(stderr, "(TODO) Syntax error: Expected function closing parenthese ')'\n");
    return NULL;
  }

  return call;
}

struct t_token *parser_next(struct t_parser *parser) {
  return scanner_next(&parser->scanner);
}

struct t_token *parser_token(struct t_parser *parser) {
  return scanner_token(&parser->scanner);
}
