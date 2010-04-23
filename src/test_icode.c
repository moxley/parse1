#include <stdio.h>
#include "parser.h"

int main(void) {
  struct t_parser parser;
  //struct t_token *token;

  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }
  
  printf("Calling parse()\n");
  
  parse(&parser);
  
  parser_close(&parser);
  printf("Done\n");
  
  exit(0);
}
