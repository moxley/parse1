#include <stdio.h>
#include "scanner.h"

int main(void) {
  struct t_scanner scanner;
  struct t_token *token;
  int i = 0;

  if (scanner_init(&scanner, stdin)) {
    fprintf(stderr, "Failed to initialize scanner\n");
    return 1;
  }

  do {
    printf("Top of loop\n");
    if ((token = scanner_next(&scanner)) == NULL) {
      fprintf(stderr, "An error occurred during parsing: errno: %d\n", scanner.error);
      break;
    }
    
    printf("%s\n", scanner_format(&scanner));
    i++;
  } while (token->type != TT_EOF && i < 30);
    
  scanner_close(&scanner);
  printf("Done.\n");
  
  return 0;
}
