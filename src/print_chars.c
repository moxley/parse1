#include <stdio.h>
#include "parse.h"
#include "filebuf.h"

int main(int argc, char *argv[]) {
  FILE *in;
  struct t_filebuf filebuf;
  char c;
  char esc_char_buf[3];
  
  printf("print_chars\n");
  if (argc < 2) {
    in = stdin;
  }
  else {
    in = fopen(argv[1], "r");
  }

  filebuf_init(&filebuf, in);
  do {
    c = filebuf_getc(&filebuf);
    if (filebuf.error) {
      fprintf(stderr, "Error during call to filebuf_getc(): %d\n", filebuf.error);
      return 1;
    }
    if (c == EOF) {
      printf("EOF\n");
      break;
    }
    else {
      util_escape_char(esc_char_buf, c);
      printf("c: '%s', row: %d, col: %d\n", esc_char_buf, filebuf.row, filebuf.col);
    }
    
  } while (1);
  
  filebuf_close(&filebuf);
  printf("After filebuf_close()\n");
  
  return 0;
}
