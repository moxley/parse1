#include <stdio.h>
#include "parse.h"
#include "filebuf.h"

int main(int argc, char *argv[]) {
  FILE *in;
  
  printf("print_lines\n");
  if (argc < 2) {
    fprintf(stderr, "Missing filename.\n");
    return 2;
  }
  
  return 0;
}
