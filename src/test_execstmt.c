#include <stdio.h>
#include "exec.h"

int main(int argc, char** argv) {
  struct t_exec exec;
  int i = 0;
  struct t_value *res;
  
  if (exec_init(&exec, stdin) < 0) {
    fprintf(stderr, "Failed to exec\n");
    exit(1);
  }
  
  do {
    res = exec_stmt(&exec);
    if (res == NULL) {
      fprintf(stderr, "Call to exec_eval() failed\n");
      break;
    }
    printf("result: %s\n", format_value(res));
    i++;
  } while (parser_token(&exec.parser)->type != TT_EOF && i < 8);
  
  exec_close(&exec);

  printf("Done.\n");
  return 0;
}
