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
  exec->stack_bottom = NULL;
  exec->stack_top = NULL;
  exec->stack_size = 0;
  return 0;
}

struct t_expr * exec_push(struct t_exec *exec, struct t_expr *expr) {
  assert(exec);
  assert(expr);
  if (!exec->stack_bottom) {
    exec->stack_bottom = expr;
    exec->stack_top = expr;
    exec->stack_size = 1;
    expr->prev = NULL;
  }
  else {
    exec->stack_top->next = expr;
    expr->prev = exec->stack_top;
    exec->stack_top = expr;
    exec->stack_size++;
  }
  expr->next = NULL;
  return expr;
}

struct t_expr * exec_pop(struct t_exec *exec) {
  struct t_expr *expr;
  
  assert(exec);
  if (!exec->stack_top) {
    return NULL;
  }
  expr = exec->stack_top;
  exec->stack_top = expr->prev;
  if (exec->stack_top) {
    exec->stack_top->next = NULL;
  }
  else {
    exec->stack_bottom = NULL;
  }
  exec->stack_size--;
  expr->prev = NULL;
  assert(!expr->next);
  
  return expr;
}

int exec_close(struct t_exec *exec) {
  struct t_expr *expr, *next;
  
  expr = exec->stack_bottom;
  while (expr) {
    next = expr->next;
    parser_expr_destroy(expr);
    free(expr);
  }
  
  return 0;
}

/*
 * Add a function.
 */
void exec_addfunc(struct t_func *func) {
  struct item *item;
  
  if (!lastfunc) {
    firstfunc = llist_newitem((void *) func);
    lastfunc = firstfunc;
  }
  else {
    item = llist_newitem(func);
    llist_append(lastfunc, item);
    lastfunc = item;
  }
}

/*
 * Find a function by name
 */
struct t_func * exec_funcbyname(char *name) {
  struct item *item;
  struct t_func *func;
  
  item = firstfunc;
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
  func = exec_funcbyname(call->name);
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
      if (res == arg) {
        /*
         * Pull the arg back out of the stack and into the function args
         */
        exec_pop(exec);
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
    res = malloc(sizeof(struct t_expr));
    parser_expr_init(res, EXP_NULL);
    if (func->invoke(func, call, res)) {
      fprintf(stderr, "(TODO) Error in native function: %s()\n", func->name);
      return NULL;
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
