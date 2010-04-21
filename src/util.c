#include <stdlib.h>
#include <string.h>
#include "util.h"

/*
 * Linked list
 */
struct item * llist_newitem(void *value) {
  struct item *item;
  
  item = malloc(sizeof(struct item));
  memset(item, 0, sizeof(struct item));
  item->value = value;
  
  return item;
}

void llist_free(struct item *first) {
  struct item *item, *next;
  
  item = first;
  while (item) {
    next = item->next;
    free(item);
    item = next;
  }
}

void llist_prepend(struct item *item, struct item *newitem) {
  item->prev = newitem;
  newitem->next = item;
}

void llist_append(struct item *item, struct item *newitem) {
  item->next = newitem;
  newitem->prev = item;
}

void llist_remove(struct item *item) {
  if (item->prev) item->prev->next = item->next;
  if (item->next) item->next->prev = item->prev;
}

void stack_init(struct stack *stack) {
  stack->bottom = NULL;
  stack->top = NULL;
  stack->size = 0;
}

void stack_empty(struct stack *stack) {
  if (stack->bottom) llist_free(stack->bottom);
  stack->size = 0;
}

int stack_push(struct stack *stack, void *item) {
  struct item *stackitem;
  
  if (!stack->bottom) {
    stack->bottom = llist_newitem(item);
    stack->top = stack->bottom;
  }
  else {
    stackitem = llist_newitem(item);
    llist_append(stack->top, stackitem);
    stack->top = stackitem;
  }
  return ++stack->size;
}

void *stack_pop(struct stack *stack) {
  void *value;
  struct item *top;
  
  if (!stack->top) {
    return NULL;
  }
  value = stack->top->value;
  top = stack->top;
  stack->top = top->prev;
  if (!stack->top) stack->bottom = NULL;
  llist_remove(top);
  
  return value;
}

void *stack_top(struct stack *stack) {
  return stack->top ? stack->top->value : NULL;
}
