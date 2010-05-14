/*
 * Test parsing to icode.
 */
#include <stdio.h>
#include "parser.h"
#include "util.h"

int main(void) {
  struct t_parser parser;
  
  debug_level = 1;

  if (parser_init(&parser, stdin)) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }
  parser.max_output = 200;
  
  parse(&parser);
  
  printf("Done with parse()\n");
  
  parser_close(&parser);
  printf("Done\n");
  
  exit(0);
}
