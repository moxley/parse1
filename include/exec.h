#ifndef exec_h
#define exec_h

#include "parser.h"
#include "util.h"

#define EXEC_SCRATCH 1024

extern struct item *firstfunc;
extern struct item *lastfunc;

/* Execution environment */
struct t_exec {
  struct t_parser parser;
  struct list stack;
  struct list functions;
  struct list vars;
  struct item *current;
  struct list formats;
};

int exec_init(struct t_exec *exec, FILE *in);
int exec_close(struct t_exec *exec);
struct t_expr * exec_push(struct t_exec *exec, struct t_expr *expr);
struct t_expr * exec_pop(struct t_exec *exec);

struct t_expr * exec_eval(struct t_exec *exec, struct t_expr *expr);
struct t_expr * exec_eval_stmt(struct t_exec *exec);

struct t_value * exec_stmt(struct t_exec *exec);
int exec_statements(struct t_exec *exec);
int exec_run(struct t_exec *exec);
struct t_value * exec_icode(struct t_exec *exec, struct t_icode *icode);

/*
 * Variables
 */
struct t_var * var_new(char *name, struct t_value *clonefrom);
void var_close(struct t_var *var);
struct t_var * var_lookup(struct t_exec *exec, char *name);

void exec_addfunc(struct t_exec *exec, struct t_func *func);
struct t_func * exec_funcbyname(struct t_exec *exec, char *name);
struct t_expr * exec_invoke(struct t_exec *exec, struct t_expr *expr);

void print_stack(struct t_exec *exec);

#endif
