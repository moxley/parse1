#include <stdio.h>
#include "exec.h"

struct t_expr * hello(struct t_func *func, struct t_fcall *call) {
  
  printf("Hello!\n");
  return NULL;
}

struct t_expr * myprint(struct t_func *func, struct t_fcall *call) {
  struct t_expr *arg;
  struct t_expr_num *num;
  
  arg = call->firstarg;
  while (arg) {
    if (arg->type == EXP_NUM) {
      num = (struct t_expr_num *) arg->detail;
      printf("%d", num->value);
    }
    else {
      printf("(%s)", arg->name);
    }
    arg = arg->next;
  }
  
  return NULL;
}

int main(int argc, char** argv) {
  struct t_parser parser;
  struct t_expr *expr, *ret;
  struct t_func func_hello;
  struct t_func func_print;
  int i = 0;
  
  /* Add a function definition */
  func_hello.name = "hello";
  func_hello.invoke = &hello;
  exec_addfunc(&func_hello);

  func_print.name = "print";
  func_print.invoke = &myprint;
  exec_addfunc(&func_print);
  
  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    exit(1);
  }
  
  do {
    expr = parser_expr_parse(&parser);
    //printf("Statement: %s\n", parser_expr_fmt(expr));
    ret = exec_stmt(expr);
    i++;
  } while (parser_token(&parser)->type != EOF && i < 3);
  
  parser_close(&parser);
  printf("Done.\n");
  return 0;
}
