/*
 * Test parsing to icode.
 */
#include <stdio.h>
#include "parser.h"
#include "util.h"

int main(int argc, char *argv[]) {
  struct t_parser parser;
  struct item *item;
  struct t_icode *icode;
  
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
  
  printf("ICODES:\n");
  item = parser.output.first;
  while (item) {
    icode = (struct t_icode *) item->value;
    printf("[%d] %s\n", icode->addr, format_icode(&parser, icode));
    item = item->next;
  }

  parser_close(&parser);
  printf("Done\n");
  
  exit(0);
}
