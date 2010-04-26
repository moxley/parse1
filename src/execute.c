#include <stdio.h>
#include "exec.h"

int hello(struct t_func *func, struct t_fcall *call, struct t_expr *res) {  
  printf("Hello!\n");
  return 0;
}

int myprintln(struct t_func *func, struct t_fcall *call, struct t_expr *res) {
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
  printf("\n");
  
  return 0;
}

int myadd(struct t_func *func, struct t_fcall *call, struct t_expr *res) {
  struct t_expr *arg;
  struct t_expr_num *num;
  int sum = 0;
  
  arg = call->firstarg;
  while (arg) {
    if (arg->type == EXP_NUM) {
      num = (struct t_expr_num *) arg->detail;
      sum += num->value;
    }
    else {
      //printf("(%s)", arg->name);
    }
    arg = arg->next;
  }
  parser_expr_init(res, EXP_NUM);
  ((struct t_expr_num *) res->detail)->value = sum;
  
  return 0;
}

int main(int argc, char** argv) {
  struct t_exec exec;
  struct t_parser parser;
  struct t_expr *expr;
  struct t_expr *res;
  struct t_func func_hello;
  struct t_func func_print;
  struct t_func func_add;
  struct t_token *token;
  int i = 0;
  
  if (exec_init(&exec, stdin) < 0) {
    fprintf(stderr, "Failed in initialize exec\n");
    exit(1);
  }
  
  /* Add a function definition */
  func_hello.name = "hello";
  func_hello.invoke = &hello;
  exec_addfunc(&exec, &func_hello);

  func_print.name = "println";
  func_print.invoke = &myprintln;
  exec_addfunc(&exec, &func_print);

  func_add.name = "add";
  func_add.invoke = &myadd;
  exec_addfunc(&exec, &func_add);
  
  do {
    expr = parser_parse(&parser);
    //printf("Parsed expression: %s\n", parser_expr_fmt(expr));
    res = exec_eval(&exec, expr);
    if (res == NULL) {
      fprintf(stderr, "Call to exec_eval() failed\n");
      break;
    }
    //printf("Result: %s\n", parser_expr_fmt(res));
    if (!exec_pop(&exec)) {
      fprintf(stderr, "exec_pop() should have returned an expression.\n");
      break;
    }
    token = parser_token(&parser);
    while (token->type == TT_EOL) {
      token = parser_next(&parser);
    }
    i++;
  } while (parser_token(&parser)->type != TT_EOF && i < 8);
  
  parser_close(&parser);

  exec_close(&exec);

  printf("Done.\n");
  return 0;
}
