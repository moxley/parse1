#include <stdio.h>
#include "exec.h"
#include "util.h"

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
  struct t_func *func;
  
  debug_level = 3;
  
  do {
    if (exec_init(&exec, stdin) < 0) {
      fprintf(stderr, "Failed to exec\n");
      break;
    }
  
    func = func_new("funcA");
    func->invoke = &myfunc;
    exec_addfunc(&exec, func);
    
    if (exec_statements(&exec) < 0) {
      fprintf(stderr, "exec_statements() failed\n");
      break;
    }
    
    item = exec.vars.first;
    if (item) {
      printf("Vars:\n");
      while (item) {
        var = (struct t_var *) item->value;
        printf("  %s=%d\n", var->name, var->value->intval);
        item = item->next;
      }
    }
    
  } while (0);
  
  exec_close(&exec);

  printf("Done.\n");
  return 0;
}
