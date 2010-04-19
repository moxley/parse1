#include <stdio.h>
#include "scanner.h"

#define INPUT_BUF_SIZE 1024
#define BUF_SIZE (INPUT_BUF_SIZE * 2)

int main(void) {
  char input_buf[INPUT_BUF_SIZE+1];
  char buf[BUF_SIZE+1];
  FILE *in = stdin;
  int len;

  do {
    len = fread(input_buf, sizeof(char), INPUT_BUF_SIZE, in);
	if (len >= INPUT_BUF_SIZE) {
	  input_buf[INPUT_BUF_SIZE] = '\0';
	}
	else {
	  input_buf[len] = '\0';
	}
	printf("input_buf after read: %s\n", input_buf);
    if (util_escape_string(buf, BUF_SIZE, input_buf)) {
      fprintf(stderr, "Input string longer than working buffer\n");
    }
  } while (!feof(in) && !ferror(in));
  printf("escaped: \"%s\"\n", buf);

  return 0;
}
