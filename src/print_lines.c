#include <stdio.h>
#include "parse.h"
#include "filebuf.h"

int main(int argc, char *argv[]) {
  FILE *in;
  struct t_filebuf filebuf;
  
  printf("print_lines\n");
  if (argc < 2) {
    in = stdin;
  }
  else {
    in = fopen(argv[1], "r");
  }

  filebuf_init(&filebuf, in);
  do {
    if (filebuf_getline(&filebuf)) {
      fprintf(stderr, "Failed on call to filebuf_getline()\n");
      filebuf_close(&filebuf);
      return 1;
    }
    printf("line: '%s'\n", filebuf.line);
    
  } while (filebuf.len);
  
  filebuf_close(&filebuf);
  printf("After filebuf_close()\n");
  
  return 0;
}
