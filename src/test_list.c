#include <stdio.h>
#include <string.h>
#include "util.h"

#define FAIL_MSG "FAIL: %s, on line %d\n"
#define assert_true(v, msg) if(!(v)){printf((FAIL_MSG), (msg), __LINE__);}

int main(void) {
  struct list list;
  char *value;

  printf("Test the list API\n");
  list_init(&list);
  list_push(&list, "one");
  list_push(&list, "two");
  assert_true(list.size == 2, "list.size should be 2");

  value = (char *) list_pop(&list);
  assert_true(value, "list_pop() should have returned a value");
  assert_true(strcmp(value, "two") == 0, "Didn't get expected value \"two\"");
  assert_true(list_size(&list) == 1, "List size should be 1");

  value = (char *) list_pop(&list);
  assert_true(value, "list_pop() should have returned a value");
  assert_true(strcmp(value, "one") == 0, "Didn't get expected value \"one\"");
  assert_true(list_size(&list) == 0, "List size should be 2");

  value = (char *) list_pop(&list);
  assert_true(value == NULL, "list_pop() should have returned NULL");
  assert_true(list_size(&list) == 0, "List size should be zero");

  printf("Done.\n");

  return 0;
}

