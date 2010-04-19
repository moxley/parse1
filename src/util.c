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

