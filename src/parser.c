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

char *icodes[] = {
  "NOP",
  "PUSH",
  "POP",
  "FCALL",
  "ADD",
  "SUB",
  "MUL",
  "DIV",
  "ASSIGN",
  "EQ",
  "NE",
};

const char *value_types[] = {
  "NULL",
  "BOOL",
  "INT",
  "FLOAT",
  "STRING",
  "OBJECT",
  "VAR",
  "FCALL"
};
int value_types_len = sizeof(value_types) / sizeof(char *);

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
char *nullexprstr = "<#expr: {type: NULL}>";
struct t_expr nullexpr;
struct t_expr noexpr;

int parser_init(struct t_parser *parser, FILE *in) {
  memset(parser, 0, sizeof(struct t_parser));
  noexpr.type = EXP_NONE;
  noexpr.format = &parser_none_fmt;
  parser->stmt = &noexpr;
  parser->errors[0] = (struct t_parse_error *) 0;
  parser->error = PARSER_ERR_NONE;
  if (scanner_init(&(parser->scanner), in)) return 1;
  list_init(&parser->output);
  return 0;
}

int parser_close(struct t_parser *parser) {
  int i;
  struct item *item;
  struct t_icode *icode;
  
  /* Free the scanner */
  scanner_close(&(parser->scanner));

  /* Free the parser errors */
  for (i=0; parser->errors[i]; i++) {
    free(parser->errors[i]);
  }

  /* Free the output */
  item = parser->output.first;
  icode = (struct t_icode *) item->value;
  while (item) {
    icode = (struct t_icode *) item->value;
    icode_close(icode);
    free(icode);
    item = item->next;
  }

  list_empty(&parser->output);

  return 0;
}

char * parser_format(struct t_parser *parser) {
  snprintf(parser->formatbuf, PARSER_FORMAT_BUF_SIZE, "<#parser: {error: %s, stmt: %s, scanner: %s}>",
    parse_error_names[parser->error],
    parser_expr_fmt(parser->stmt),
    scanner_format(&(parser->scanner)));
  return parser->formatbuf;
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
//  stmt->next = NULL;
//  if (!parser->first) {
//    parser->first = stmt;
//  }
//  else {
//    parser->stmt->next = stmt;
//  }
//  parser->stmt = stmt;
  
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
    return nullexprstr;
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
//  struct t_token *token;
//
//  token = parser_token(parser);
//  if (token->type == TT_EOF) {
//    fprintf(stderr, "%s(): Unexpected end of file\n", __FUNCTION__);
//    return NULL;
//  }
//
//  expr = parser_parse_stmt(parser);
//  if (!expr) return NULL;
//  token = parser_token(parser);
//  if (!token) return NULL;
//  
//  if (token->type != TT_EOL && token->type != TT_EOF) {
//    fprintf(stderr, "Unexpected token. Expected EOL or EOF\n");
//    do {
//      token = parser_next(parser);
//      if (!token) return NULL;
//    } while (token->type != TT_EOL && token->type != TT_EOF);
//  }
//  
//  while (token->type == TT_EOL) {
//    token = parser_next(parser);
//  }
//  
  return expr;
}

int parse(struct t_parser *parser)
{
  struct t_token *token = NULL;
  
  token = parser_token(parser);
  
  do {
    if (parse_stmt(parser) < 0) return -1;
    
    token = parser_token(parser);
    if (token->type == TT_EOF) {
      break;
    }
    else if (token->type != TT_EOL && token->type != TT_SEMI) {
      break;
    }
    
    if (token->type == TT_EOL || token->type == TT_SEMI) {
      create_icode_append(parser, I_POP, NULL);
    }
    
    while (token->type == TT_EOL || token->type == TT_SEMI) {
      token = parser_next(parser);
    }
  } while (token->type != TT_EOF);
  
  if (token->type != TT_EOF) {
    fprintf(stderr, "Unexpected token: %s", token_format(token));
    return -1;
  }
  
  return 0;
}

int parse_stmt(struct t_parser *parser)
{
  struct t_token *token = NULL;

  token = parser_token(parser);
  
  parse_expr(parser);
  token = parser_token(parser);
  if (token->type == TT_EQUAL) {
    if (parse_assign(parser) < 0) return -1;
  }

  return 0;
}

//struct t_expr * parser_parse_stmt(struct t_parser *parser) {
//  struct t_token *token = NULL;
//  struct t_expr *expr = NULL;
//  
//  token = parser_token(parser);
//  if (token->type == TT_NAME) {
//    token = parser_next(parser);
//    parser_pushtoken(parser);
//    if (token->type == TT_EQUAL) {
//      parse_parse_assignment(parser);
//    }
//    else {
//      expr = parser_parse_simple(parser);
//    }
//  }
//  else {
//    expr = parser_parse_simple(parser);
//  }
//  
//  return expr;
//}

int parse_if(struct t_parser *parser)
{
  return -1;
}

//struct t_expr * parse_parse_assignment(struct t_parser *parser) {
//  return NULL;
//}

int parse_assign(struct t_parser *parser)
{
  struct t_token *token;
  
  token = parser_token(parser);
  if (token->type != TT_EQUAL) {
    return -1;
  }
  
  while (token->type == TT_EQUAL) {
    token = parser_next(parser);
    if (parse_expr(parser) < 0) return -1;
    create_icode_append(parser, I_ASSIGN, NULL);
    token = parser_token(parser);
  }
  
  return 0;
}

int compare_multiple_strings(const char *source, char **list)
{
  if (!source) return 0;
  int i;
  for (i=0; list[i]; i++) {
    if (strcmp(source, list[i]) == 0) return 1;
  }
  return 0;
}

int parse_expr(struct t_parser *parser)
{
  //char *ops[] = {"==", "!=", "<", ">", "<=", ">=", "\0"};
  char *ops[] = {"==", "!=", NULL};
  struct t_token *token;
  
  token = parser_token(parser);
  
  if (parse_simple(parser) < 0) return -1;
  token = parser_token(parser);
  if (!compare_multiple_strings(token->buf, ops)) {
    return 0;
  }
  
  while (compare_multiple_strings(token->buf, (char **) ops)) {
    token = parser_next(parser);
    if (parse_simple(parser) < 0) return -1;
    if (strcmp("==", token->buf)) {
      create_icode_append(parser, I_EQ, NULL);
    }
    else if (strcmp("!=", token->buf)) {
      create_icode_append(parser, I_NE, NULL);
    }
    token = parser_token(parser);
  }
  return 0;
}

//struct t_expr * parser_parse_simple(struct t_parser *parser) {
//  return parser_parse_term(parser);
//}

int parse_simple(struct t_parser *parser)
{
  struct t_token *token;
  char *ops[] = {"-", "+", NULL};
  int minus = 0;
  int itype;
  
  token = parser_token(parser);
  if (strcmp(token->buf, "-") == 0) {
    minus = 1;
  }
  if (parse_term(parser) < 0) return -1;
  if (minus) {
    create_icode_append(parser, I_PUSH, create_num_from_int(-1));
    create_icode_append(parser, I_MUL, NULL);
  }
  
  do {
    token = parser_token(parser);
    if (!compare_multiple_strings(token->buf, ops)) {
      break;
    }
    itype = strcmp(token->buf, "-")==0 ? I_SUB : I_ADD;
    token = parser_next(parser);

    if (parse_term(parser) < 0) return -1;
    create_icode_append(parser, itype, NULL);
  } while (1);
  
  return 0;
}

//struct t_expr * parser_parse_term(struct t_parser *parser) {
//  struct t_token *token;
//  struct t_expr *termexpr = NULL;
//  struct t_binom *term;
//  
//  parser_parse_factor(parser);
//  token = parser_token(parser);
//  while (token->type == TT_PLUS || token->type == TT_PLUS) {
//    termexpr = malloc(sizeof(struct t_expr));
//    parser_expr_init(termexpr, EXP_TERM);
//    term = (struct t_binom *) termexpr->detail;
//    term->op = token;
//    term->left = parser_popexpr(parser);
//    
//    parser_next(parser);
//    
//    parser_parse_factor(parser);
//    term->right = parser_popexpr(parser);
//    
//    token = parser_token(parser);
//  }
//  
//  return termexpr;
//}

int parse_term(struct t_parser *parser)
{
  struct t_token *token;
  char *ops[] = {"*", "/", NULL};
  int itype;
  
  token = parser_token(parser);
  
  if (parse_factor(parser) < 0) return -1;
  token = parser_token(parser);
  
  do {
    if (!token->buf || !compare_multiple_strings(token->buf, ops)) {
      return 0;
    }
    itype = strcmp("/", token->buf) == 0 ? I_DIV : I_MUL;
    token = parser_next(parser);
    if (parse_factor(parser) < 0) return -1;
    create_icode_append(parser, itype, NULL);
    token = parser_token(parser);
  } while (1);
  
  return 0;
}

//struct t_expr * parser_parse_factor(struct t_parser *parser) {
//  struct t_token *token;
//  struct t_expr *expr = NULL;
//  
//  token = parser_token(parser);
//  if (token->type == TT_NUM) {
//    // TODO Do number expression
//    expr = parser_num_parse(parser);
//  }
//  else if (token->type == TT_NAME) {
//    token = parser_next(parser);
//    if (token->type == TT_PARENL) {
//      parser_pushtoken(parser);
//      expr = parser_fcall_parse(parser);
//    }
//    else if (token->type == TT_EQUAL) {
//      parser_pushtoken(parser);
//      expr = parser_term_parse(parser);
//    }
//    else {
//      fprintf(stderr, "%s(): Unknown expression: TT_NAME, %s\n", __FUNCTION__, token_types[token->type]);
//      while (token->type != TT_EOL && token->type != TT_EOF) {
//        token = parser_next(parser);
//        if (token->type == TT_EOL) {
//          token = parser_next(parser);
//        }
//      }
//      return NULL;
//    }
//  }
//  else {
//    expr = &noexpr;
//    parser_next(parser);
//  }
//  
//  return expr;
//}

int parse_factor(struct t_parser *parser)
{
  struct t_token *token;
  
  token = parser_token(parser);
  if (token->type == TT_PARENL) {
    token = parser_next(parser);
    if (parse_expr(parser) < 0) return -1;
    token = parser_token(parser);
    if (token->type != TT_PARENR) {
      return 0;
    }
    token = parser_next(parser);
  }
  else if (token->type == TT_NUM) {
    if (parse_num(parser) < 0) return -1;
  }
  else if (token->type == TT_NAME) { // variable
    if (parse_name(parser) < 0) return -1;
  }
  else {
    return -1;
  }

  token = parser_token(parser);
  
  return 0;
}

int parse_num(struct t_parser *parser)
{
  struct t_token *token;
  token = parser_token(parser);
  create_icode_append(parser, I_PUSH, create_num_from_str(token->buf));
  parser_next(parser);
  return 0;
}

int parse_name(struct t_parser *parser)
{
  struct t_token *token;
  char *name;
  int i;
  struct t_icode *icode;
  
  token = parser_token(parser);
  name = token->buf;
  token = parser_next(parser);
  if (token->type != TT_PARENL) {
    create_icode_append(parser, I_PUSH, create_var(name));
  }
  else {
    token = parser_next(parser);
    if (token->type == TT_PARENR) {
      i = 0;
      token = parser_next(parser);
    }
    else {
      for (i=0; ;i++) {
        if (parse_expr(parser) < 0) return -1;
        token = parser_token(parser);
        if (token->type == TT_COMMA) {
          token = parser_next(parser);
        }
        else if (token->type == TT_PARENR) {
          token = parser_next(parser);
          break;
        }
        else {
          fprintf(stderr, "Missing closing ')' in call to %s(). token: %s\n", name, token_format(token));
          return -1;
        }
      }
    }
    struct t_value *fcall;
    fcall = create_fcall(name, i + 1);
    icode = create_icode_append(parser, I_FCALL, fcall);
  }
  
  return 0;
}

struct t_icode * icode_new(int type, struct t_value *operand)
{
  struct t_icode *icode;
  
  icode = malloc(sizeof(struct t_icode));
  icode->type = type;
  icode->operand = operand;
  icode->formatbuf = NULL;
  
  return icode;
}

void icode_close(struct t_icode *icode)
{
  if (icode->formatbuf) {
    free(icode->formatbuf);
  }
  if (icode->operand) {
    free(icode->operand);
  }
}

struct t_icode * create_icode_append(struct t_parser *parser, int type, struct t_value *operand)
{
  struct t_icode *icode;
  
  icode = icode_new(type, operand);
  debug(1, "%s(): Appending icode: %s\n", __FUNCTION__, format_icode(parser, icode));
  list_push(&parser->output, icode);
  
  return icode;
}

char * format_icode(struct t_parser *parser, struct t_icode *icode)
{
  char buf[PARSER_SCRATCH_BUF + 1];
  int len;
  struct t_icode *opnd1, *opnd2;
  struct item *item;
  char *toobig = "<#icode {TOO_BIG}>";
  
  if (icode->operand) {
    len = snprintf(buf, PARSER_SCRATCH_BUF, "(%s %s)",
      icodes[icode->type],
      format_value(icode->operand));
  }
  else {
    item = parser->output.last;
    while (item && item->value != icode) {
      item = item->prev;
    }
    if (!item) {
      len = snprintf(buf, PARSER_SCRATCH_BUF, "(%s)", icodes[icode->type]);
    }
    else {
      assert(item->prev);
      assert(item->prev->prev);
      assert(item->prev->value);
      assert(item->prev->prev->value);
      opnd2 = (struct t_icode *) item->prev->value;
      opnd1 = (struct t_icode *) item->prev->prev->value;
      len = snprintf(buf, PARSER_SCRATCH_BUF, "(%s %s %s)",
        icodes[icode->type],
        (opnd1->operand ? format_value(opnd1->operand) : format_icode(parser, opnd1)),
        (opnd2->operand ? format_value(opnd2->operand) : format_icode(parser, opnd2)));
    }
  }
  
  if (len > PARSER_SCRATCH_BUF) {
    icode->formatbuf = malloc(sizeof(char) * (strlen(toobig) + 1));
    strcpy(icode->formatbuf, toobig);
  }
  else {
    icode->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(icode->formatbuf, buf);
  }
  
  return icode->formatbuf;
}

char * format_value(struct t_value *value)
{
  char buf[PARSER_SCRATCH_BUF + 1];
  int len;
  char *toobigval = "TOO_BIG";
  char *toobig = "<#value {TOO_BIG}>";
  char valuebuf[PARSER_SCRATCH_BUF + 2 + 1];
  int show_literal = 0;

  if (!value) return "NULL";
  
  /*
   * Format the value as would be displayed in source code.
   */
  if (value->type == VAL_INT) {
    len = snprintf(valuebuf, PARSER_SCRATCH_BUF, "%d", value->intval);
    if (len > PARSER_SCRATCH_BUF) {
      strcpy(valuebuf, toobigval);
    }
    show_literal = 1;
  }
  else if (value->type == VAL_STRING) {
    valuebuf[0] = '"';
    len = util_escape_string(&valuebuf[1], PARSER_SCRATCH_BUF - 2, value->stringval);
    if (len > PARSER_SCRATCH_BUF - 2) {
      strcpy(valuebuf, toobigval);
    }
    else {
      valuebuf[len+1] = '"';
      valuebuf[len+2] = '\0';
    }
    show_literal = 1;
  }
  else if (value->type == VAL_VAR) {
    len = snprintf(valuebuf, PARSER_SCRATCH_BUF, "%s", value->name);
    if (len > PARSER_SCRATCH_BUF) {
      strcpy(valuebuf, toobigval);
    }
    show_literal = 1;
  }
  else if (value->type == VAL_FCALL) {
    len = snprintf(valuebuf, PARSER_SCRATCH_BUF, "%s(argc=%d)", value->name, value->argc);
    if (len > PARSER_SCRATCH_BUF) {
      strcpy(valuebuf, toobigval);
    }
    show_literal = 1;
  }
  else {
    snprintf(valuebuf, PARSER_SCRATCH_BUF, "(%s)", value_types[value->type]);
  }
  
  if (show_literal) {
    value->formatbuf = malloc(sizeof(char) * (len + 1));
    strcpy(value->formatbuf, valuebuf);
  }
  else {
    len = snprintf(buf, PARSER_SCRATCH_BUF, "<#value: {type: %s, value: %s}>", value_types[value->type], valuebuf);
    if (len > PARSER_SCRATCH_BUF) {
      value->formatbuf = malloc(sizeof(char) * (strlen(toobig) + 1));
      strcpy(value->formatbuf, toobig);
    }
    else {
      value->formatbuf = malloc(sizeof(char) * (len + 1));
      strcpy(value->formatbuf, buf);
    }
  }
  
  return value->formatbuf;
}

void value_init(struct t_value *value, int type)
{
  value->type = type;
  value->intval = 0;
  value->floatval = 0.0;
  value->stringval = NULL;
  value->len = 0;
  value->formatbuf = NULL;
  value->name = NULL;
  value->argc = 0;
}

void value_close(struct t_value *value)
{
  if (value->stringval) free(value->stringval);
  if (value->formatbuf) free(value->formatbuf);
  if (value->name) free(value->name);
}

struct t_value * create_num_from_int(int v)
{
  struct t_value *value;
  
  value = malloc(sizeof(struct t_value));
  value_init(value, VAL_INT);
  value->intval = v;
  
  return value;
}

struct t_value * create_num_from_str(char * v)
{
  return create_num_from_int(atoi(v));
}

struct t_value * create_str(char *str)
{
  struct t_value *value;
  
  value = malloc(sizeof(struct t_value));
  value_init(value, VAL_STRING);
  value->len = strlen(str);
  value->stringval = malloc(sizeof(char) * (value->len + 1));
  strcpy(value->stringval, str);
  
  return value;
}

struct t_value * create_var(char *name)
{
  struct t_value *iden;
  
  iden = malloc(sizeof(struct t_value));
  value_init(iden, VAL_VAR);
  
  iden->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(iden->name, name);
  
  return iden;
}

struct t_value * create_fcall(char *name, int argc)
{
  struct t_value *fcall;
  
  fcall = malloc(sizeof(struct t_value));
  value_init(fcall, VAL_FCALL);
  
  fcall->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(fcall->name, name);
  fcall->argc = argc;
  
  return fcall;
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
//  struct t_token *token;
//  struct t_fcall *call;
  struct t_expr *expr = NULL;
//  struct t_expr *argexpr = NULL;
//  struct t_expr *prev = NULL;
//
//  expr = malloc(sizeof(struct t_expr));
//  parser_expr_init(expr, EXP_FCALL);
//  call = (struct t_fcall *) expr->detail;
//
//  token = parser_token(parser);
//  if (!token) return NULL;
//  if (token->type != TT_NAME) {
//      // TODO return ERROR expression
//      fprintf(stderr, "(TODO) Syntax error. Expected function name. Got %s: '%s'\n", token_types[token->type], token->buf);
//      return NULL;
//  }
//  if (call->name) free(call->name);
//  call->name = malloc(sizeof(char) * (strlen(token->buf) + 1));
//  strcpy(call->name, token->buf);
//
//  token = parser_next(parser);
//  if (!token) return NULL;
//  if (token->type != TT_PARENL) {
//    fprintf(stderr, "(TODO) %s(): Syntax error: Expected function opening parenthese '('. Got %s: %s\n", __FUNCTION__, token_types[token->type], token->buf);
//    return NULL;
//  }
//
//  token = parser_next(parser);
//  if (!token) return NULL;
//  if (token->type == TT_PARENR) {
//    token = parser_next(parser);
//  }
//  else {
//    while (1) {
//      if (token->type == TT_EOF) {
//        fprintf(stderr, "(TODO) (in call to %s) Unexpected end of file", call->name);
//        return NULL;
//      }
//      argexpr = parser_parse_simple(parser);
//      if (!call->firstarg) {
//        call->firstarg = argexpr;
//      }
//      else {
//        prev->next = argexpr;
//      }
//      call->argcount++;
//      prev = argexpr;
//
//      token = parser_token(parser);
//      if (!token) return NULL;
//      if (token->type == TT_PARENR) {
//        parser_next(parser);
//        break;
//      }
//      else if (token->type != TT_COMMA) {
//        fprintf(stderr, "(TODO) (in call to %s) Syntax error: Expected function argument or closing parenthese ')'. Got: %s\n", call->name, token_format(token));
//        return NULL;
//      }
//      else {
//        parser_next(parser);
//      }
//    }
//  }
//
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
