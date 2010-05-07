/*
 * Test the interpreter.
 */

#include <stdio.h>
#include "exec.h"
#include "util.h"
#include "corelib.h"

int main(int argc, char* argv[]) {
  struct t_exec exec;
  
  do {
    if (exec_init(&exec, stdin) < 0) {
      fprintf(stderr, "Failed to exec\n");
      break;
    }
    core_apply(&exec);
    
    exec.parser.max_output = 100;
  
    if (exec_statements(&exec) < 0) {
      break;
    }
    
  } while (0);
  
  exec_close(&exec);

  return 0;
}
