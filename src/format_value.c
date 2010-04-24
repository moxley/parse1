#include <stdio.h>
#include <string.h>
#include "parser.h"

int main(int argc, char *argv[])
{
  char *typearg;
  char *valarg;
  struct t_value value;
  int type = -1;
  int i;
  
  if (argc < 3) {
    fprintf(stderr, "Usage: format_value TYPE VALUE\n");
    exit(2);
  }
  
  typearg = argv[1];
  for (i=0; i < value_types_len; i++) {
    if (strcmp(typearg, value_types[i]) == 0) {
      type = i;
      break;
    }
  }
  
  if (type < 0) {
    fprintf(stderr, "Unknown type: %s\n", typearg);
    exit(2);
  }
  
  valarg = argv[2];
  value_init(&value, type);
  if (type == VAL_INT) {
    value.intval = atoi(valarg);
  }
  else if (type == VAL_STRING) {
    value.stringval = valarg;
    value.len = strlen(valarg);
  }
  else {
    fprintf(stderr, "format_value does not support VAL_%s\n", typearg);
    exit(2);
  }
  printf("value: %s\n", format_value(&value));
  
  exit(0);
}