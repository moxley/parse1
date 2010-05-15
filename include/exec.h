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
  struct list values;
};

/* ICode Operation */
struct t_icode_op {
  int opnd_count;
  struct t_value * (*op0)(struct t_exec *exec, struct t_icode *icode);
  struct t_value * (*op1)(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd);
  struct t_value * (*op2)(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
};

extern const struct t_icode_op operations[];
extern const int operations_len;

int exec_main(int argc, char* argv[]);
int exec_init(struct t_exec *exec, FILE *in);
int exec_close(struct t_exec *exec);
struct t_expr * exec_push(struct t_exec *exec, struct t_expr *expr);
struct t_expr * exec_pop(struct t_exec *exec);

struct t_value * exec_stmt(struct t_exec *exec);
int exec_statements(struct t_exec *exec);
int exec_run(struct t_exec *exec);
struct t_value * exec_icode(struct t_exec *exec, struct t_icode *icode);

struct t_value * exec_i_nop(struct t_exec *exec, struct t_icode *icode);
struct t_value * exec_i_pop(struct t_exec *exec, struct t_icode *icode);
struct t_value * exec_i_push(struct t_exec *exec, struct t_icode *icode);
struct t_value * exec_i_fcall(struct t_exec *exec, struct t_icode *fcall);
struct t_value * exec_i_jmp(struct t_exec *exec, struct t_icode *jmp);
struct t_value * exec_i_jz(struct t_exec *exec, struct t_icode *jmp);

struct t_value * exec_i_assign(struct t_exec *exec, struct t_icode *icode);
struct t_value * exec_i_add(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_sub(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_mul(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_div(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_eq(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_ne(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_lt(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_gt(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_le(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);
struct t_value * exec_i_ge(struct t_exec *exec, struct t_icode *icode, struct t_value *opnd1, struct t_value *opnd2);

/*
 * Variables
 */
struct t_var * var_new(char *name, struct t_value *clonefrom);
void var_close(struct t_var *var);
struct t_var * var_lookup(struct t_exec *exec, char *name);

void exec_addfunc(struct t_exec *exec, struct t_func *func);
struct t_func * exec_addfunc2(struct t_exec *exec, char *name, int (*fn)(struct t_func *func, struct list *args, struct t_value *ret));
struct t_func * exec_funcbyname(struct t_exec *exec, char *name);
struct t_expr * exec_invoke(struct t_exec *exec, struct t_expr *expr);

#endif
