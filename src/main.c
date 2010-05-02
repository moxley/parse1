/*
 * Test the interpreter.
 */

#include <stdio.h>
#include "exec.h"
#include "util.h"

/*
 * C-based function that will be called by interpreted code.
 *
 * Simply returns the value of the first argument.
 */
int my_println(struct t_func *func, struct list *args, struct t_value *ret) {
  struct t_value *arg;
  
  arg = args->first->value;
  printf("%d\n", arg->intval);

  return 0;
}

int main(int argc, char* argv[]) {
  struct t_exec exec;
  struct t_func *func;
  
  do {
    if (exec_init(&exec, stdin) < 0) {
      fprintf(stderr, "Failed to exec\n");
      break;
    }
    
    exec.parser.max_output = 100;
  
    func = func_new("println");
    func->invoke = &my_println;
    exec_addfunc(&exec, func);
    
    if (exec_statements(&exec) < 0) {
      break;
    }
    
  } while (0);
  
  exec_close(&exec);

  return 0;
}
