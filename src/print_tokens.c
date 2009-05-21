#include <stdio.h>
#include "parse.h"

int main(void) {
  struct t_scanner scanner;
  int value;

  if (scanner_init(&scanner, stdin)) {
    fprintf(stderr, "Failed to initialize scanner\n");
    return 1;
  }

  do {
    if (scanner_token(&scanner)) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", scanner.error);
      break;
    }
    scanner_print(&scanner);
  } while (scanner.token.type != TT_EOF);
  scanner_close(&scanner);
  printf("Done.\n");
  return 0;
}

