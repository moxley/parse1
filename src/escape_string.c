#include <stdio.h>
#include "parse.h"

#define INPUT_BUF_SIZE 30
#define BUF_SIZE 60

int main(void) {
  char input_buf[INPUT_BUF_SIZE+1];
  char buf[BUF_SIZE+1];
  FILE *in = stdin;

  do {
    fread(input_buf, 1, INPUT_BUF_SIZE, in);
    if (util_escape_string(buf, BUF_SIZE, input_buf)) {
      fprintf(stderr, "Input string longer than working buffer\n");
    }
  } while (!feof(in) && !ferror(in));
  printf("escaped: \"%s\"\n", buf);

  return 0;
}
