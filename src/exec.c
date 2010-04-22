#include <string.h>
#include <assert.h>
#include "exec.h"
#include "util.h"

struct item *firstfunc = NULL;
struct item *lastfunc = NULL;

/*
 * Initialize an execution environment.
 */
int exec_init(struct t_exec *exec) {
  list_init(&exec->statements);
  list_init(&exec->stack);
  list_init(&exec->functions);
  return 0;
}

int exec_close(struct t_exec *exec) {
  struct item *item;
  
  item = exec->statements.first;
  while (item) {
    parser_expr_destroy((struct t_expr *) item->value);
    free(item->value);
    item = item->next;
  }

  list_empty(&exec->statements);
  list_empty(&exec->stack);
  list_empty(&exec->functions);
  
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

/*
 * Invoke a function.
 */
struct t_expr * exec_invoke(struct t_exec *exec, struct t_expr *expr) {
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
