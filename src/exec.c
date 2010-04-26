#include <string.h>
#include <assert.h>
#include "exec.h"
#include "util.h"
#include "parser.h"

struct item *firstfunc = NULL;
struct item *lastfunc = NULL;

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
  
  item = exec->stack.first;
  while (item) {
    value_close(item->value);
    item = item->next;
  }
  list_empty(&exec->stack);
  
  list_empty(&exec->functions);
  
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

struct t_expr * exec_push(struct t_exec *exec, struct t_expr *expr) {
  assert(exec);
  assert(expr);
  
  list_push(&exec->stack, expr);
  
  return expr;
}

struct t_expr * exec_pop(struct t_exec *exec) {
  assert(exec);
  return (struct t_expr *) list_pop(&exec->stack);
}

/*
 * Add a function.
 */
void exec_addfunc(struct t_exec *exec, struct t_func *func) {
  list_push(&exec->functions, func);
}

/*
 * Find a function by name
 */
struct t_func * exec_funcbyname(struct t_exec *exec, char *name) {
  struct item *item;
  struct t_func *func;
  
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

/*
 * Evaluate a parsed expression.
 *
 * Leaves the result on the stack and returns it too.
 */
struct t_expr * exec_eval(struct t_exec *exec, struct t_expr *expr) {
  struct t_expr *res = NULL;
  
  if (!expr) {
    expr = exec_pop(exec);
    if (!expr) {
      fprintf(stderr, "%s(): No expression on the stack\n", __FUNCTION__);
      return NULL;
    }
  }
  
  if (expr->type == EXP_FCALL) {
    res = exec_invoke(exec, expr);
  }
  else if (expr->type == EXP_NUM) {
    exec_push(exec, expr);
    res = expr;
  }
  else {
    fprintf(stderr, "%s(): Don't know how to handle expression: %s\n", __FUNCTION__, parser_expr_fmt(expr));
    return NULL;
  }
  
  return res;
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
  
  if (parse_stmt(&exec->parser) < 0) {
    res = NULL;
  }
  else {
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
  
  if (!exec->current) {
    exec->current = exec->parser.output.first;
  }
  while (exec->current) {
    icode = (struct t_icode *) exec->current->value;
    debug(1, "Executing icode type=%s\n", icodes[icode->type]);
    if (!exec_icode(exec, icode)) {
      debug(1, "%s(): returning -1 at line %d\n", __FUNCTION__, __LINE__);
      return -1;
    }
    exec->current = exec->current->next;
  }
  
  return 0;
}

struct t_value * exec_icode(struct t_exec *exec, struct t_icode *icode)
{
  struct t_value *opnd1, *opnd2;
  struct t_value *ret;
  int intval;
  
  if (icode->type == I_PUSH) {
    assert(icode->operand);
    list_push(&exec->stack, icode->operand);
    ret = icode->operand;
  }
  else if (icode->type == I_POP) {
    ret = list_pop(&exec->stack);
  }
  else if (icode->type == I_FCALL) {
    debug(3, "%s(): icode->type == I_FCALL\n", __FUNCTION__);
    debug(3, "Stack size at line %d: %d\n", __LINE__, exec->stack.size);
    debug(3, "Top of stack at line %d: %s\n", __LINE__, format_value(list_last(&exec->stack)));
    struct t_func * func;
    func = exec_funcbyname(exec, icode->operand->name);
    
    struct list a, b;
    int i;
    list_init(&a);
    list_init(&b);
    for (i=0; i < icode->operand->argc; i++) {
      list_push(&a, list_pop(&exec->stack));
    }
    for (i=0; i < icode->operand->argc; i++) {
      list_push(&b, list_pop(&a));
    }
    
    /* TODO Invoke the function */
    ret = create_num_from_int(22);
    
    list_push(&exec->stack, ret);
  }
  else {
    opnd2 = list_pop(&exec->stack);
    opnd1 = list_pop(&exec->stack);
    if (icode->type == I_ADD) {
      intval = opnd1->intval + opnd2->intval;
    }
    else if (icode->type == I_SUB) {
      intval = opnd1->intval - opnd2->intval;
    }
    else if (icode->type == I_MUL) {
      intval = opnd1->intval * opnd2->intval;
    }
    else if (icode->type == I_DIV) {
      if (opnd2->intval == 0) {
        fprintf(stderr, "Divide by zero: %d / %d", opnd1->intval, opnd2->intval);
        return NULL;
      }
      intval = opnd1->intval / opnd2->intval;
    }
    else if (icode->type == I_EQ) {
      intval = opnd1->intval == opnd2->intval;
    }
    else if (icode->type == I_ASSIGN) {
      struct t_var *var;
      if (opnd1->type != VAL_VAR) {
        fprintf(stderr, "Left side of assignment must be a variable. Got %s=%d instead.\n", value_types[opnd1->type], opnd1->intval);
        return NULL;
      }
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
      
      if (var->value->type == VAL_INT) {
        var->value->intval = opnd2->intval;
      }
      else {
        fprintf(stderr, "Don't know how to assign %s type value\n", value_types[opnd2->type]);
        return NULL;
      }
    }
    else {
      fprintf(stderr, "Don't know how to handle %s icode\n", icodes[icode->type]);
      return NULL;
    }
    ret = create_num_from_int(intval);
    list_push(&exec->stack, ret);
  }
  
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
      return item->value;
    }
    item = item->next;
  }
  
  return NULL;
}

/*
 * Invoke a function.
 */
struct t_expr * exec_invoke(struct t_exec *exec, struct t_expr *expr)
{
  struct t_expr *arg;
  struct t_expr *res;
  struct t_fcall *call;
  struct t_func *func;
  struct t_expr *prev, *next;
  
  call = (struct t_fcall *) expr->detail;
  func = exec_funcbyname(exec, call->name);
  if (!func) {
    fprintf(stderr, "(TODO) %s: Function %s() is not defined.\n", __FUNCTION__, call->name);
    return NULL;
  }
  
  /*
   * Evaluate arguments
   *
   * Replaces arguments with evaluated arguments, if they can be reduced.
   */
  arg = call->firstarg;
  while (arg) {
    prev = arg->prev;
    next = arg->next;
    res = exec_eval(exec, arg);
    if (!res) {
      return NULL;
    }
    else {
      exec_pop(exec);
      if (res == arg) {
        /*
         * Pull the arg back out of the stack and into the function args
         */
        arg->prev = prev;
        arg->next = next;
      }
      else {
        if (arg->prev) arg->prev->next = res;
        res->prev = arg->prev;
        res->next = arg->next;
        if (arg->next) arg->next->prev = res;
        if (arg == call->firstarg) call->firstarg = res;
        parser_expr_destroy(arg);
        free(arg);
      }
    }
    arg = arg->next;
  }
  
  if (func->invoke) {
    res = calloc(1, sizeof(struct t_expr));
    if (func->invoke(func, call, res)) {
      fprintf(stderr, "(TODO) Error in native function: %s()\n", func->name);
      return NULL;
    }
    if (!res->type) {
      parser_expr_init(res, EXP_NULL);
    }
    exec_push(exec, res);
  }
  else if (func->first) {
    /* TODO Put parameters in namespace */
    fprintf(stderr, "%s(): func->first is not implemented\n", __FUNCTION__);
    //stmt_item = func->first;
    //while (stmt_item) {
    //  stmt = (struct t_expr *) stmt_item->value;
    //  res = exec_eval(exec, stmt);
    //  if (res == NULL) {
    //    fprintf(stderr, "(TODO) %s: Failed to evaluate statement in function %s\n", __FUNCTION__, func->name);
    //    return NULL;
    //  }
    //  exec_pop(exec);
    //}
  }
  else {
    fprintf(stderr, "(TODO) %s: Function %s() is missing a body", __FUNCTION__, func->name);
    return NULL;
  }
  
  return res;
}

//char * format_stack(struct t_exec *exec)
//{
//  struct item *item;
//  struct t_value *value;
//  char buf[EXEC_SCRATCH+1];
//  int len;
//  char *format;
//  const char *sep = ", ";
//  int seplen = sizeof(sep) / sizeof(char);
//  const char *stack_fmt = "<#stack {%s}>";
//  
//  item = exec->stack.first;
//  while (item) {
//    value = (struct t_value *) item->value;
//    format = format_value(value);
//    len += strlen(format);
//    if (len < EXEC_SCRATCH) {
//      strcpy(buf, format);
//    }
//  }
//  
//  return buf;
//}
//