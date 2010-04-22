#ifndef exec_h
#define exec_h

#include "parser.h"
#include "util.h"

extern struct item *firstfunc;
extern struct item *lastfunc;

/* Execution environment */
struct t_exec {
  struct list stack;
  struct list functions;
};

int exec_init(struct t_exec *exec);
int exec_close(struct t_exec *exec);
struct t_expr * exec_push(struct t_exec *exec, struct t_expr *expr);
struct t_expr * exec_pop(struct t_exec *exec);

struct t_expr * exec_eval(struct t_exec *exec, struct t_expr *expr);

void exec_addfunc(struct t_exec *exec, struct t_func *func);
struct t_func * exec_funcbyname(struct t_exec *exec, char *name);
struct t_expr * exec_invoke(struct t_exec *exec, struct t_expr *expr);

#endif
