#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "exec.h"
#include "util.h"
#include "parser.h"

const struct t_icode_op operations[] = {
  {0, &exec_i_nop, NULL, NULL},
  {0, &exec_i_push, NULL, NULL},
  {0, &exec_i_pop, NULL, NULL},
  {0, &exec_i_fcall, NULL, NULL},
  {2, NULL, NULL, &exec_i_add},
  {2, NULL, NULL, &exec_i_sub},
  {2, NULL, NULL, &exec_i_mul},
  {2, NULL, NULL, &exec_i_div},
  {0, &exec_i_assign, NULL, NULL},
  {2, NULL, NULL, &exec_i_eq},
  {2, NULL, NULL, &exec_i_ne},
  {0, &exec_i_jmp, NULL, NULL},
  {0, &exec_i_jz, NULL, NULL},
  {1, &exec_i_jst, NULL, NULL},
  {2, NULL, NULL, &exec_i_lt},
  {2, NULL, NULL, &exec_i_gt},
  {2, NULL, NULL, &exec_i_le},
  {2, NULL, NULL, &exec_i_ge}
};
const int operations_len = sizeof(operations) / sizeof(struct t_icode_op);

/*
 * Initialize an execution environment.
 */
int exec_init(struct t_exec *exec, FILE *in) {
  if (parser_init(&exec->parser, in)) return -1;
  list_init(&exec->stack);
  list_init(&exec->functions);
  list_init(&exec->vars);
  list_init(&exec->formats);
  exec->current = NULL;
  return 0;
}

int exec_close(struct t_exec *exec) {
  struct item *item;
  
  parser_close(&exec->parser);
  
  item = exec->functions.first;
  while (item) {
    func_free(item->value);
    item = item->next;
  }
  list_empty(&exec->functions);

  item = exec->values.first;
  while (item) {
    value_free(item->value);
    item = item->next;
  }
  list_empty(&exec->values);
  
  item = exec->vars.first;
  while (item) {
    var_close(item->value);
    item = item->next;
  }
  list_empty(&exec->vars);

  item = exec->formats.first;
  while (item) {
    var_close(item->value);
    item = item->next;
  }
  list_empty(&exec->formats);
  
  return 0;
}

/*
 * Move functions from parser to exec
 */
void exec_get_funcs(struct t_exec *exec)
{
  struct item *item;

  item = exec->parser.functions.first;
  if (item) {
    printf("Copying parser functions to exec functions\n");
    item->prev = exec->functions.last; // May be NULL
    if (exec->functions.last) {
      exec->functions.last->next = item;
    }
    exec->functions.last = exec->parser.functions.last;
    if (!exec->functions.first) {
      exec->functions.first = item;
    }
    exec->parser.functions.first = NULL;
    exec->parser.functions.last = NULL;
  }
}

/*
 * Add a function.
 */
//void exec_addfunc(struct t_exec *exec, struct t_func *func) {
//  list_push(&exec->functions, func);
//}

/*
 * Alternate for exec_addfunc()
 */
struct t_func * exec_addfunc2(struct t_exec *exec, char *name, int (*fn)(struct t_func *func, struct list *args, struct t_value *ret))
{
  struct t_func *func;
  func = func_new(name);
  func->invoke = fn;
  //exec_addfunc(exec, func);
  list_push(&exec->functions, func);
  return func;
}

/*
 * Find a function by name
 */
struct t_func * exec_funcbyname(struct t_exec *exec, char *name) {
  struct item *item;
  struct t_func *func;
  
  printf("exec_funcbyname() name=%s\n", name);
  item = exec->functions.first;
  while (item) {
    func = (struct t_func *) item->value;
    if (strcmp(func->name, name) == 0) {
      return func;
    }
    item = item->next;
  }
  
  return NULL;
}

int exec_statements(struct t_exec *exec)
{
  if (parse(&exec->parser) < 0) {
    return -1;
  }
  
  if (exec_run(exec) < 0) {
    return -1;
  }
  
  return 0;
}

struct t_value * exec_stmt(struct t_exec *exec)
{
  struct t_value *res;
  struct t_token *token;
  
  debug(1, "%s(): Calling parse_stmt()\n", __FUNCTION__);
  if (parse_stmt(&exec->parser) < 0) {
    res = NULL;
  }
  else {
    debug(1, "%s(): Calling exec_run()\n", __FUNCTION__);
    if (exec_run(exec) < 0) {
      res = NULL;
    }
    else {
      res = list_pop(&exec->stack);
    }
  }
  
  token = parser_token(&exec->parser);
  
  if (!res) {
    while (token->type != TT_EOL && token->type != TT_EOF) {
      token = parser_next(&exec->parser);
    }
  }
  
  while (token->type == TT_EOL) {
    token = parser_next(&exec->parser);
  }
  
  return res;
}

int exec_run(struct t_exec *exec)
{
  struct t_icode *icode;
  struct t_value *ret;

  exec_get_funcs(exec);

  if (!exec->current) {
    exec->current = exec->parser.output.first;
  }
  while (exec->current) {
    icode = (struct t_icode *) exec->current->value;
    ret = exec_icode(exec, icode);
    if (!ret) {
      debug(1, "%s(): returning -1 at line %d\n", __FUNCTION__, __LINE__);
      return -1;
    }
    exec->current = exec->current->next;
  }
  
  return 0;
}

struct t_value * exec_icode(struct t_exec *exec, struct t_icode *icode)
{
  struct t_value *ret;
  struct t_value *opnd1, *opnd2;
  struct t_value *val1, *val2;
  struct t_icode_op op;
  struct t_var *var;

  debug(1, "%s(): Executing icode addr=%d: %s\n", __FUNCTION__, icode->addr, format_icode(&exec->parser, icode));

  if (icode->type < 0 || icode->type >= operations_len) {
    fprintf(stderr, "Invalid operation type (value=%d)\n", icode->type);
    return NULL;
  }

  op = operations[icode->type];

  if (op.opnd_count == 0) {
    ret = op.op0(exec, icode);
  }
  else if (op.opnd_count == 1) {
    opnd1 = list_pop(&exec->stack);
    assert(opnd1);
    if (opnd1->type == VAL_VAR) {
      var = var_lookup(exec, opnd1->name);
      val1 = var->value;
      assert(opnd1);
    }
    else {
      val1 = opnd1;
    }

    ret = op.op1(exec, icode, val1);
    list_push(&exec->stack, ret);
  }
  else if (op.opnd_count == 2) {
    opnd2 = list_pop(&exec->stack);
    assert(opnd2);
    opnd1 = list_pop(&exec->stack);
    assert(opnd1);

    if (opnd1->type == VAL_VAR) {
      var = var_lookup(exec, opnd1->name);
      val1 = var->value;
      assert(val1);
    }
    else {
      val1 = opnd1;
    }

    if (opnd2->type == VAL_VAR) {
      var = var_lookup(exec, opnd2->name);
      val2 = var->value;
      assert(val2);
    }
    else {
      val2 = opnd2;
    }

    ret = op.op2(exec, icode, val1, val2);
    list_push(&exec->stack, ret);
  }
  else {
    fprintf(stderr, "Invalid number of operations (%d) for op '%s'\n", op.opnd_count, icodes[icode->type]);
    return NULL;
  }
  
  return ret;
}

struct t_value * exec_i_pop(struct t_exec *exec, struct t_icode *icode)
{
  debug(3, "%s(): Before pop, stack size: %d\n", __FUNCTION__, exec->stack.size);
  return list_pop(&exec->stack);
}

struct t_value * exec_i_nop(struct t_exec *exec, struct t_icode *icode)
{
  return &nullvalue;
}

struct t_value * exec_i_push(struct t_exec *exec, struct t_icode *icode)
{
  assert(icode->operand);
  list_push(&exec->stack, icode->operand);
  return icode->operand;
}

struct t_value * exec_i_fcall(struct t_exec *exec, struct t_icode *fcall)
{
  struct list a, args;
  int i;
  struct t_value *ret = &nullvalue;
  struct t_func * func;
  struct t_value *opnd;
  struct item *item;
  int current_addr;
  int offset;

  debug(3, "%s(): Stack size at line %d: %d\n", __FUNCTION__, __LINE__, exec->stack.size);
  debug(3, "%s(): Top of stack at line %d: %s\n", __FUNCTION__, __LINE__, format_value(list_last(&exec->stack)));

  func = exec_funcbyname(exec, fcall->operand->name);
  if (!func) {
    fprintf(stderr, "Error: Function %s() is not defined, on Line %d.\n", fcall->operand->name, fcall->token->row+1);
    return NULL;
  }

  /* Prepare the arguments */
  list_init(&a);
  list_init(&args);
  for (i=0; i < fcall->operand->argc; i++) {
    list_push(&a, list_pop(&exec->stack));
  }
  for (i=0; i < fcall->operand->argc; i++) {
    list_push(&args, list_pop(&a));
  }

  if (func->invoke) {
    DBG(2, "Calling C function");
    ret = calloc(1, sizeof(struct t_value));
    list_push(&exec->values, ret);
    if (func->invoke(func, &args, ret) < 0) {
      fprintf(stderr, "(TODO) Error in native function: %s()\n", func->name);
      return NULL;
    }
    list_push(&exec->stack, ret);
  }
  else {
    DBG(2, "Calling local function");

    // Calculate the offset
    offset = 0;
    item = exec->parser.output.first;
    printf("func start: %d\n", func->start);
    current_addr = 0;
    while (item != exec->current) {
      item = item->next;
      current_addr++;
      if (item == exec->current) {
        printf("Found current\n");
      }
    }
    offset = (func->start + 1) - current_addr;

    opnd = create_num_from_int(offset);
    list_push(&exec->stack, opnd);
    exec_i_jst(exec, NULL);
  }
  
  return ret;
}

struct t_value * exec_i_jmp(struct t_exec *exec, struct t_icode *jmp)
{
  if (exec_jump(exec, jmp->operand->intval) < 0) {
    return NULL;
  }

  return &nullvalue;
}

int exec_jump(struct t_exec *exec, int offset)
{
  int i;
  debug(3, "%s(): Doing jump. offset=%d\n", __FUNCTION__, offset);
  
  if (offset > 0) {
    // Increment offset-1 because the instruction pointer will get incremented anyway.
    for (i=0; i < offset-1; i++) {
      exec->current = exec->current->next;
      assert(exec->current);
    }
  }
  else if (offset < 0) {
    for (i=offset-1; i < 0; i++) {
      exec->current = exec->current->prev;
      assert(exec->current);
    }
  }
  
  return 0;
}

struct t_value * exec_i_jz(struct t_exec *exec, struct t_icode *jmp)
{
  struct t_value *ret = &nullvalue;
  
  ret = list_pop(&exec->stack);
  if (ret->intval == 0) {
    exec_i_jmp(exec, jmp);
  }
  
  return ret;
}

/*
 * Jump to location identified by the top of stack.
 */
struct t_value * exec_i_jst(struct t_exec *exec, struct t_icode *jmp)
{
  struct t_value *ret = &nullvalue;

  ret = list_pop(&exec->stack);
  assert(ret);
  exec_jump(exec, ret->intval);

  return ret;
}

struct t_value * exec_i_assign(struct t_exec *exec, struct t_icode *icode)
{
  struct t_var *var;
  struct t_value *ret = NULL;
  struct t_value *opnd1, *opnd2;
  
  debug(2, "%s(): Begin\n", __FUNCTION__);
  
  opnd2 = list_pop(&exec->stack);
  assert(opnd2);
  opnd1 = list_pop(&exec->stack);
  assert(opnd1);
  
  debug(3, "%s(): Got operands from stack\n", __FUNCTION__);

  if (opnd2->type == VAL_VAR) {
    debug(3, "%s(): Fetching value for opnd2\n", __FUNCTION__);
    var = var_lookup(exec, opnd2->name);
    opnd2 = var->value;
    assert(opnd2);
  }

  if (opnd1->type != VAL_VAR) {
    fprintf(stderr, "Left side of assignment must be a variable. Got %s=%d instead.\n", value_types[opnd1->type], opnd1->intval);
    return NULL;
  }
  debug(3, "%s(): Looking up opnd1 var name=%s\n", __FUNCTION__, opnd1->name);
  var = var_lookup(exec, opnd1->name);
  if (var) {
    if (var->value->type != opnd2->type) {
      fprintf(stderr, "Type mismatch when assigning new value: %s = %s\n", opnd1->name, value_types[opnd2->type]);
      return NULL;
    }
  }
  else {
    var = var_new(opnd1->name, opnd2);
    list_push(&exec->vars, var);
  }
  debug(3, "%s(): Copying value to variable\n", __FUNCTION__);

  if (var->value->type == VAL_INT) {
    var->value->intval = opnd2->intval;
    ret = create_num_from_int(var->value->intval);
    list_push(&exec->values, ret);
    debug(3, "%s(): New int val: %d\n", __FUNCTION__, ret->intval);
  }
  else if (var->value->type == VAL_STRING) {
    var->value->stringval = opnd2->stringval;
    ret = var->value;
    debug(3, "%s(): Assigned string: %s\n", __FUNCTION__, ret->stringval);
  }
  else {
    fprintf(stderr, "Don't know how to assign %s type value\n", value_types[opnd2->type]);
    return NULL;
  }
  
  list_push(&exec->stack, ret);

  return ret;
}

struct t_value * exec_i_add(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret = NULL;
  char buf[PARSER_SCRATCH_BUF+1];
  
  if (opnd1->type == VAL_INT) {
    if (opnd2->type != VAL_INT) {
      fprintf(stderr, "%s(): Don't know how to add a %s value to an int value.\n", __FUNCTION__, value_types[opnd2->type]);
    }
    ret = create_num_from_int(opnd1->intval + opnd2->intval);
    list_push(&exec->values, ret);
  }
  else if (opnd1->type == VAL_STRING) {
    int opnd2len;
    char *opnd2str;
    
    if (opnd2->type == VAL_STRING) {
      opnd2str = opnd2->stringval;
    }
    else if (opnd2->type == VAL_INT) {
      snprintf(buf, PARSER_SCRATCH_BUF, "%d", opnd2->intval);
      opnd2str = buf;
    }
    else {
      fprintf(stderr, "%s(): Don't know how to concatenate a %s value to a string.", __FUNCTION__, value_types[opnd2->type]);
      return NULL;
    }
    opnd2len = strlen(opnd2str);
    
    ret = create_value(VAL_STRING);
    list_push(&exec->values, ret);
    ret->stringval = malloc(sizeof(char) * (strlen(opnd1->stringval) + opnd2len + 1));
    strcpy(ret->stringval, opnd1->stringval);
    strcat(ret->stringval, opnd2str);
  }
  else {
    fprintf(stderr, "%s(): Don't know how to concatenate a %s value.\n", __FUNCTION__, value_types[opnd1->type]);
    return NULL;
  }
  
  return ret;
}

struct t_value * exec_i_sub(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;

  ret = create_num_from_int(opnd1->intval - opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_mul(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;

  ret = create_num_from_int(opnd1->intval * opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_div(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  if (opnd2->intval == 0) {
    fprintf(stderr, "Divide by zero: %d / %d", opnd1->intval, opnd2->intval);
    return NULL;
  }
  ret = create_num_from_int(opnd1->intval / opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_eq(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  ret = create_num_from_int(opnd1->intval == opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_ne(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  ret = create_num_from_int(opnd1->intval != opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_lt(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  ret = create_num_from_int(opnd1->intval < opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_gt(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  ret = create_num_from_int(opnd1->intval > opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_le(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  ret = create_num_from_int(opnd1->intval <= opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_value * exec_i_ge(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2)
{
  struct t_value *ret;
  
  ret = create_num_from_int(opnd1->intval >= opnd2->intval);
  list_push(&exec->values, ret);
  
  return ret;
}

struct t_var * var_new(char *name, struct t_value *clonefrom)
{
  struct t_var *var;
  
  var = malloc(sizeof(struct t_var));
  var->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(var->name, name);
  
  var->value = malloc(sizeof(struct t_value));
  memcpy(var->value, clonefrom, sizeof(struct t_value));
  
  return var;
}

void var_close(struct t_var *var)
{
  if (var->name) free(var->name);
  if (var->value) free(var->value);
}

struct t_var * var_lookup(struct t_exec *exec, char *name)
{
  struct item *item;
  
  item = exec->vars.first;
  while (item) {
    if (strcmp(((struct t_var *) item->value)->name, name) == 0) {
      return ((struct t_var *) item->value);
    }
    item = item->next;
  }
  
  return NULL;
}
