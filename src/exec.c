#include <string.h>
#include "exec.h"
#include "util.h"

struct item *firstfunc = NULL;
struct item *lastfunc = NULL;

/*
 * Execute a statement.
 */
struct t_expr * exec_stmt(struct t_expr *stmt) {
  struct t_func *func;
  struct t_fcall *call;
  struct t_expr *ret = NULL;
  
  if (!stmt) {
    return NULL;
  }
  
  if (stmt->type == EXP_FCALL) {
    call = (struct t_fcall *) stmt->detail;
    func = exec_funcbyname(call->name);
    if (!func) {
      fprintf(stderr, "(TODO) Function %s() is not defined.\n", call->name);
      return NULL;
    }
    ret = exec_invoke(func, call);
  }
  
  return ret;
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
 * Invoke a function.
 */
struct t_expr * exec_invoke(struct t_func *func, struct t_fcall *call) {
  struct t_expr *ret = NULL;
  struct item *stmt_item;
  struct t_expr *stmt;
  
  if (func->invoke) {
    ret = func->invoke(func, call);
  }
  else if (func->first) {
    /* TODO Put parameters in namespace */
    stmt_item = func->first;
    while (stmt_item) {
      stmt = (struct t_expr *) stmt_item->value;
      ret = exec_stmt(stmt);
    }
  }
  else {
    fprintf(stderr, "(TODO) Function %s() is missing a body", func->name);
    return NULL;
  }
  
  return ret;
}
