/*
 * Test the interpreter.
 */

#include <stdio.h>
#include "exec.h"
#include "util.h"
#include "corelib.h"

/*
 * C-based function that will be called by interpreted code.
 *
 * Simply returns the value of the first argument.
 */
int myfunc(struct t_func *func, struct list *args, struct t_value *ret) {
  struct t_value *arg;
  
  arg = args->first->value;

  ret->type = VAL_INT;
  ret->intval = arg->intval;
  
  return 0;
}

int main(int argc, char* argv[]) {
  struct t_exec exec;
  struct item *item;
  struct t_var *var;
  
  if (argc > 1) {
    debug_level = atoi(argv[1]);
  }
  else {
    debug_level = 1;
  }
  printf("debug_level: %d\n", debug_level);
  
  do {
    if (exec_init(&exec, stdin) < 0) {
      fprintf(stderr, "Failed to exec\n");
      break;
    }
    
    core_apply(&exec);

    exec.parser.max_output = 100;
  
    exec_addfunc2(&exec, "funcA", &myfunc);
    
    if (exec_statements(&exec) < 0) {
      fprintf(stderr, "exec_statements() failed\n");
      break;
    }
    
    item = exec.vars.first;
    if (item) {
      printf("Vars:\n");
      while (item) {
        var = (struct t_var *) item->value;
        if (var->value->type == VAL_STRING) {
          printf("  %s=%s\n", var->name, var->value->stringval);
        }
        else {
          printf("  %s=%d\n", var->name, var->value->intval);
        }
        item = item->next;
      }
    }
    
  } while (0);
  
  exec_close(&exec);

  printf("Done.\n");
  return 0;
}
