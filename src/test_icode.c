/*
 * Test parsing to icode.
 */
#include <stdio.h>
#include "parser.h"
#include "util.h"

int main(int argc, char *argv[]) {
  struct t_parser parser;
  
  if (argc > 1) {
    debug_level = atoi(argv[1]);
  }
  else {
    debug_level = 1;
  }

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
