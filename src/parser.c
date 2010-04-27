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
  "JMP",
  "JZ"
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
char statement_buf[STATEMENT_FORMAT_BUF_SIZE];

int parser_init(struct t_parser *parser, FILE *in) {
  memset(parser, 0, sizeof(struct t_parser));
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
  snprintf(parser->formatbuf, PARSER_FORMAT_BUF_SIZE, "<#parser: {error: %s, scanner: %s}>",
    parse_error_names[parser->error],
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
    fprintf(stderr, "%s(): Unexpected token: %s\n", __FUNCTION__, token_format(token));
    return -1;
  }
  
  return 0;
}

int parse_stmt(struct t_parser *parser)
{
  struct t_token *token = NULL;
  int ret;
  debug(2, "%s(): Begin\n", __FUNCTION__);

  token = parser_token(parser);
  while (token->type == TT_EOL || token->type == TT_SEMI) {
    token = parser_next(parser);
  }
  if (token->type == TT_EOF) {
    ret = 0;
  }
  else {
    if (token->type == TT_NAME && strcmp("if", token->buf) == 0) {
      ret = parse_if(parser);
    }
    else {
      parse_expr(parser);
      token = parser_token(parser);
      debug(3, "%s() line %d: Token: %s\n", __FUNCTION__, __LINE__, token_format(token));
      if (token->type == TT_EQUAL) {
        if (parse_assign(parser) < 0) return -1;
        token = parser_token(parser);
      }
      while (token->type == TT_EOL || token->type == TT_SEMI) {
        token = parser_next(parser);
      }
      debug(3, "%s(): Token before POP: %s\n", __FUNCTION__, token_format(token));
      create_icode_append(parser, I_POP, NULL);
      ret = 0;
    }
  }
  
  debug(2, "%s(): End\n", __FUNCTION__);
  
  return ret;
}

int parse_if(struct t_parser *parser)
{
  struct t_token *token;
  struct t_icode *start;
  int ret;
  debug(2, "%s(): Begin\n", __FUNCTION__);
  
  /*
  if 1 == 2
    stmt1
    stmt2
  else if 1 == 3
    stmt3
    stmt4
  else
    stmt5
    stmt6
  end
  stmt7
  */
  /*
  1. push 1
  2. push 2
  3. ==
  4. jmz +4
  5. stmt1
  6. stmt2
  7. jmp +10
  8. push 1
  9. push 3
  10. ==
  11. jmz +4
  12. stmt3
  13. stmt4
  14. jmp +2
  15. stmt5
  16. stmt6
  17. stmt7
  */
  /*
   * Parse conditional
   */
  token = parser_next(parser);
  if (parse_expr(parser) < -1) return -1;
  token = parser_token(parser);
  while (token->type == TT_EOL || token->type == TT_SEMI) {
    token = parser_next(parser);
  }
  create_icode_append(parser, I_JZ, NULL);
  
  do {
    debug(3, "%s():  Token before statement within IF block: %s\n", __FUNCTION__, token_format(token));
    debug(3, "%s():  Parsing statement within IF block.\n", __FUNCTION__);
    if (parse_stmt(parser) < -1) return -1;
    token = parser_token(parser);
    debug(3, "%s():  Token after statement within IF block: %s\n", __FUNCTION__, token_format(token));
    if (token->type == TT_EOF) {
      fprintf(stderr, "%s(): Unexpected end=of-file within IF statement.\n", __FUNCTION__);
      ret = -1;
      break;
    }
    else if (token->type == TT_NAME && strcmp("else", token->buf) == 0) {
      create_icode_append(parser, I_JMP, NULL);
      token = parser_next(parser);
      if (token->type == TT_NAME && strcmp("if", token->buf) == 0) {
        debug(3, "%s(): IF:ELSE IF\n", __FUNCTION__);
        token = parser_next(parser);
        if (parse_expr(parser) < -1) {
          ret = -1;
          break;
        }
        token = parser_token(parser);
        while (token->type == TT_EOL || token->type == TT_SEMI) {
          token = parser_next(parser);
        }
        create_icode_append(parser, I_JZ, NULL);
      }
      else {
        create_icode_append(parser, I_JZ, NULL);
        debug(3, "%s(): IF:ELSE\n", __FUNCTION__);
      }
    }
    else if (token->type == TT_NAME && strcmp("end", token->buf) == 0) {
      debug(3, "%s(): IF:END\n", __FUNCTION__);
      token = parser_next(parser);
      ret = 0;
      break;
    }
  } while (1);
  
  debug(2, "%s(): End\n", __FUNCTION__);
  return ret;
}

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
    len = snprintf(valuebuf, PARSER_SCRATCH_BUF, "var:%s", value->name);
    if (len > PARSER_SCRATCH_BUF) {
      strcpy(valuebuf, toobigval);
    }
    show_literal = 1;
  }
  else if (value->type == VAL_FCALL) {
    len = snprintf(valuebuf, PARSER_SCRATCH_BUF, "func:%s(argc=%d)", value->name, value->argc);
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

struct t_func * func_new(char *name)
{
  struct t_func *func;
  
  func = malloc(sizeof(struct t_func));
  func->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(func->name, name);
  
  return func;
}

void func_close(struct t_func *func)
{
}
