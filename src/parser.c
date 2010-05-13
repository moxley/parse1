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
struct t_value nullvalue;
struct t_value truevalue;
struct t_value falsevalue;

int parser_init(struct t_parser *parser, FILE *in) {
  memset(parser, 0, sizeof(struct t_parser));
  parser->errors[0] = (struct t_parse_error *) 0;
  parser->error = PARSER_ERR_NONE;
  parser->max_output = -1;
  if (scanner_init(&(parser->scanner), in)) return 1;
  list_init(&parser->output);
  
  value_init(&nullvalue, VAL_NULL);
  value_init(&falsevalue, VAL_BOOL);
  falsevalue.intval = 0;
  value_init(&truevalue, VAL_BOOL);
  truevalue.intval = 1;
  
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
        //scanner_init_token(&(parser->scanner), TT_ERROR);
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

struct t_token * handle_token_error(struct t_parser *parser, struct t_token *token)
{
  if (!token) {
    perror("Fatal error");
    exit(1);
  }
  else if (token->type == TT_ERROR) {
    fprintf(stderr, "Parse error: %s", parse_error_names[token->type]);
  }
  
  return token;
}

/*
 * Get next token
 */
struct t_token *parser_next(struct t_parser *parser) {
  return handle_token_error(parser, scanner_next(&parser->scanner));
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
 * Parse a normal list of statements.
 */
int parse(struct t_parser *parser)
{
  struct t_token *token = NULL;
  
  do {
    token = parser_token(parser);
  } while (token->type == TT_ERROR);
  
  do {
    if (parse_stmt(parser) < 0) return -1;
    
    token = parser_token(parser);
    if (token->type == TT_EOF) {
      break;
    }
    
    while (token->type == TT_EOL || token->type == TT_SEMI || token->type == TT_ERROR) {
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

  token = parser_token(parser);
  debug(2, "%s(): Begin. Token: %s\n", __FUNCTION__, token_format(token));
  while (token->type == TT_EOL || token->type == TT_SEMI) {
    token = parser_next(parser);
  }
  if (token->type == TT_EOF) {
    ret = 0;
  }
  else {
    if (token->type == TT_NAME && strcmp("if", token->buf) == 0) {
      ret = parse_if(parser);
      if (ret < 0) return -1;
      token = parser_token(parser);
      while (token->type == TT_EOL || token->type == TT_SEMI) {
        token = parser_next(parser);
      }
    }
    else {
      if (parse_expr(parser) < 0) return -1;
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
      if (!create_icode_append(parser, I_POP, NULL)) return -1;
      ret = 0;
    }
  }
  
  debug(2, "%s(): End\n", __FUNCTION__);
  
  return ret;
}

int parse_if(struct t_parser *parser)
{
  struct t_token *token;
  int ret = -1;
  int after_addr = -1;
  struct t_icode *jmp;
  struct t_icode *prev_jmp;
  struct t_icode *end_block_jmp;
  struct list block_ends;
  debug(2, "%s(): Begin\n", __FUNCTION__);
  //dbg(1, "begin: %s\n", "foo");
  DBG(1, "begin:", "foo");
  
  list_init(&block_ends);
  
  debug(3, "%s(): IF addr=%d\n", __FUNCTION__, parser->output.size);
  /*
   * Parse conditional
   */
  token = parser_next(parser);
  if (parse_expr(parser) < -1) return -1;
  token = parser_token(parser);
  while (token->type == TT_EOL || token->type == TT_SEMI) {
    token = parser_next(parser);
  }
  
  /*
   * Create JMP instruction.
   * It is up to the next "else", "else if" or "end" to set the jump offset.
   */
  jmp = create_icode_append(parser, I_JZ, create_num_from_int(0));
  if (!jmp) {
    goto parse_if_end;
  }
  
  debug(3, "%s():  'if' I_JZ addr=%d\n", __FUNCTION__, jmp->addr);
  prev_jmp = jmp;

  do {
    debug(3, "%s():  Token before statement within IF block: %s\n", __FUNCTION__, token_format(token));
    debug(3, "%s():  Parsing statement within IF block.\n", __FUNCTION__);
    if (parse_stmt(parser) < 0) {
      token_format(token);
      fprintf(stderr, "Syntax Error: Line %d, Column %d: Unrecognized token in IF() conditional: '%s'\n", (token->row+1), (token->col+1), token->buf);
      goto parse_if_end;
    }
    token = parser_token(parser);
    debug(3, "%s():  Token after statement within IF block: %s\n", __FUNCTION__, token_format(token));
    if (token->type == TT_EOF) {
      fprintf(stderr, "%s(): Unexpected end=of-file within IF statement.\n", __FUNCTION__);
      goto parse_if_end;
    }
    else if (token->type == TT_NAME && strcmp("else", token->buf) == 0) {
      /*
       * Found either "else" or "else if"
       */
     
      /*
       * Create JMP at the end of the previous block
       * We'll need to set the offset (end_block_jmp->operand->intval) later, so we
       * save it the jmp to a list.
       */
      end_block_jmp = create_icode_append(parser, I_JMP, create_num_from_int(0));
      if (!end_block_jmp) {
        goto parse_if_end;
      }
      debug(3, "%s():  end of block I_JMP. addr=%d\n", __FUNCTION__, end_block_jmp->addr);
      list_push(&block_ends, end_block_jmp);
    
      /*
       * Set the JMP offset for the previous conditional.
       */
      prev_jmp->operand->intval = parser->output.size - prev_jmp->addr;
      debug(3, "%s():  prev cond offset=%d\n", __FUNCTION__, prev_jmp->operand->intval);
    
      token = parser_next(parser);
      if (token->type == TT_NAME && strcmp("if", token->buf) == 0) {
        /* "else if" */
        debug(3, "%s(): IF:ELSE IF. addr=%d\n", __FUNCTION__, parser->output.size);
        token = parser_next(parser);
        if (parse_expr(parser) < -1) {
          goto parse_if_end;
        }
        token = parser_token(parser);
        while (token->type == TT_EOL || token->type == TT_SEMI) {
          token = parser_next(parser);
        }
        // Create jump for "else if"
        jmp = create_icode_append(parser, I_JZ, create_num_from_int(0));
        if (!jmp) {
          goto parse_if_end;
        }
        debug(3, "%s(): 'else if' I_JZ addr=%d\n", __FUNCTION__, jmp->addr);
        prev_jmp = jmp;
      }
      else {
        debug(3, "%s(): IF:ELSE. addr=%d\n", __FUNCTION__, parser->output.size);
        prev_jmp = NULL;
        // Nothing to do; we already set the jmp offset for the previous conditional.
      }
    }
    else if (token->type == TT_NAME && strcmp("end", token->buf) == 0) {
      debug(3, "%s(): IF:END. addr=%d\n", __FUNCTION__, parser->output.size);
    
      after_addr = parser->output.size;
      debug(3, "%s(): after_addr: %d\n", __FUNCTION__, after_addr);

      /* Set the JMP offset for the last conditional, if there was no 'else' block. */
      if (prev_jmp) {
        prev_jmp->operand->intval = after_addr - prev_jmp->addr;
        debug(3, "%s(): Set JMP offset for prev conditional (%d) to %d\n", __FUNCTION__, prev_jmp->addr, prev_jmp->operand->intval);
      }
    
      /*
       * Set the JMP offsets for each conditional block end.
       */
      struct item *item;
      item = block_ends.first;
      while (item) {
        jmp = (struct t_icode *) item->value;
        jmp->operand->intval = after_addr - jmp->addr;
        debug(3, "%s(): Set JMP offset for cond block end to %d\n", __FUNCTION__, jmp->operand->intval);
        item = item->next;
      }
    
      token = parser_next(parser);
      ret = 0;
      break;
    }
  } while (1);
  
  parse_if_end:
  
  /* Free the list */
  list_empty(&block_ends);
  
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
    if (!create_icode_append(parser, I_ASSIGN, NULL)) return -1;
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
  debug(2, "%s(): Begin. token: %s\n", __FUNCTION__, token_format(token));
  
  if (parse_simple(parser) < 0) return -1;
  token = parser_token(parser);
  if (!compare_multiple_strings(token->buf, ops)) {
    return 0;
  }
  
  while (compare_multiple_strings(token->buf, (char **) ops)) {
    token = parser_next(parser);
    if (parse_simple(parser) < 0) return -1;
    if (strcmp("==", token->buf)) {
      if (!create_icode_append(parser, I_EQ, NULL)) return -1;
    }
    else if (strcmp("!=", token->buf)) {
      if (!create_icode_append(parser, I_NE, NULL)) return -1;
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
  debug(2, "%s(): Begin. token: %s\n", __FUNCTION__, token_format(token));
  if (strcmp(token->buf, "-") == 0) {
    minus = 1;
  }
  if (parse_term(parser) < 0) return -1;
  if (minus) {
    if (!create_icode_append(parser, I_PUSH, create_num_from_int(-1))) return -1;
    if (!create_icode_append(parser, I_MUL, NULL)) return -1;
  }
  
  do {
    token = parser_token(parser);
    if (!compare_multiple_strings(token->buf, ops)) {
      break;
    }
    itype = strcmp(token->buf, "-")==0 ? I_SUB : I_ADD;
    token = parser_next(parser);

    if (parse_term(parser) < 0) return -1;
    if (!create_icode_append(parser, itype, NULL)) return -1;
  } while (1);
  
  return 0;
}

int parse_term(struct t_parser *parser)
{
  struct t_token *token;
  char *ops[] = {"*", "/", NULL};
  int itype;
  
  token = parser_token(parser);
  debug(2, "%s(): Begin. token: %s\n", __FUNCTION__, token_format(token));
  
  if (parse_factor(parser) < 0) return -1;
  token = parser_token(parser);
  
  do {
    if (!token->buf || !compare_multiple_strings(token->buf, ops)) {
      return 0;
    }
    itype = strcmp("/", token->buf) == 0 ? I_DIV : I_MUL;
    token = parser_next(parser);
    if (parse_factor(parser) < 0) return -1;
    if (!create_icode_append(parser, itype, NULL)) return -1;
    token = parser_token(parser);
  } while (1);
  
  return 0;
}

int parse_factor(struct t_parser *parser)
{
  struct t_token *token;
  
  token = parser_token(parser);
  debug(2, "%s(): Begin. token: %s\n", __FUNCTION__, token_format(token));

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
  else if (token->type == TT_STRING) {
    if (!create_icode_append(parser, I_PUSH, create_str(token->buf))) return -1;
    parser_next(parser);
  }
  else {
    // Higher-level caller needs to display the error message
    return -1;
  }

  token = parser_token(parser);
  
  return 0;
}

int parse_num(struct t_parser *parser)
{
  struct t_token *token;
  token = parser_token(parser);
  if (!create_icode_append(parser, I_PUSH, create_num_from_str(token->buf))) return -1;
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
    if (!create_icode_append(parser, I_PUSH, create_var(name))) return -1;
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
    if (!icode) return -1;
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
  icode->addr = -1;
  
  return icode;
}

void icode_close(struct t_icode *icode)
{
  if (icode->formatbuf) {
    free(icode->formatbuf);
    icode->formatbuf = NULL;
  }
  if (icode->operand) {
    value_free(icode->operand);
    icode->operand = NULL;
  }
}

struct t_icode * create_icode_append(struct t_parser *parser, int type, struct t_value *operand)
{
  struct t_icode *icode;
  struct t_token *token;
  
  if (parser->output.size >= parser->max_output) {
    fprintf(stderr, "Maximum number of icodes %d reached.", parser->output.size);
    return NULL;
  }
  
  token = parser_token(parser);
  
  icode = icode_new(type, operand);
  icode->addr = parser->output.size;
  icode->token = token;
  debug(1, "%s(): Appending icode addr=%d: %s\n", __FUNCTION__, icode->addr, format_icode(parser, icode));
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

struct t_value * create_value(int type)
{
  struct t_value *value;
  
  value = malloc(sizeof(struct t_value));
  value_init(value, type);
  
  return value;
}

void value_close(struct t_value *value)
{
  if (value->stringval) {
    free(value->stringval);
    value->stringval = NULL;
  }
  if (value->formatbuf) {
    free(value->formatbuf);
    value->formatbuf = NULL;
  }
  if (value->name) {
    free(value->name);
    value->name = NULL;
  }
}

void value_free(struct t_value *value)
{
  value_close(value);
  free(value);
}

struct t_value * create_num_from_int(int v)
{
  struct t_value *value;
  
  value = create_value(VAL_INT);
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
  
  value = create_value(VAL_STRING);
  value->len = strlen(str);
  value->stringval = malloc(sizeof(char) * (value->len + 1));
  strcpy(value->stringval, str);
  
  return value;
}

struct t_value * create_var(char *name)
{
  struct t_value *iden;
  
  iden = create_value(VAL_VAR);
  iden->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(iden->name, name);
  
  return iden;
}

struct t_value * create_fcall(char *name, int argc)
{
  struct t_value *fcall;
  
  fcall = create_value(VAL_FCALL);
  
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
  free(func->name);
}

void func_free(struct t_func *func)
{
  func_close(func);
  free(func);
}
