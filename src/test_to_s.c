#include <stdio.h>
#include "parser.h"

int main(void)
{
  //struct t_parser parser;
  struct t_value *value;
  char *str;
  
  value = create_str("Foo bar");
  str = value_to_s(value);
  
  printf("to_s: %s\n", str);
  
  return 0;
}
