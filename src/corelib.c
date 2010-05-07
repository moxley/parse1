/*
 * Core library.
 *
 * A library of built-in functions
 */
#include "exec.h"

/*
 * C-based function that will be called by interpreted code.
 *
 * Simply returns the value of the first argument.
 */
int fn_println(struct t_func *func, struct list *args, struct t_value *ret)
{
  struct t_value *arg;
  
  arg = args->first->value;
  printf("%d\n", arg->intval);

  return 0;
}

int core_apply(struct t_exec *exec)
{
  exec_addfunc2(exec, "println", &fn_println);
  return 0;
}
