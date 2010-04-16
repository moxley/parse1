#include <stdio.h>
#include "scanner.h"

int main(void) {
  struct t_scanner scanner;
  struct t_token *token;

  if (scanner_init(&scanner, stdin)) {
    fprintf(stderr, "Failed to initialize scanner\n");
    return 1;
  }

  do {
    if ((token = scanner_next(&scanner)) == NULL) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", scanner.error);
      break;
    }
    scanner_print(&scanner);
  } while (token->type != TT_EOF);
  scanner_close(&scanner);
  printf("Done.\n");
  return 0;
}
