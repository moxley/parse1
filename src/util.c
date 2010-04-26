#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"

int debug_level = 0;
FILE *debug_stream;

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

void list_init(struct list *list) {
  list->first = NULL;
  list->last = NULL;
  list->size = 0;
}

void list_empty(struct list *list) {
  if (list->first) llist_free(list->first);
  list->size = 0;
}

int list_size(struct list *list) {
  return list->size;
}

int list_push(struct list *list, void *item) {
  struct item *listitem;

  if (!list->first) {
    list->first = llist_newitem(item);
    list->last = list->first;
  }
  else {
    listitem = llist_newitem(item);
    llist_append(list->last, listitem);
    list->last = listitem;
  }
  return ++list->size;
}

void *list_pop(struct list *list) {
  void *value;
  struct item *last;
  
  if (!list->last) {
    return NULL;
  }
  value = list->last->value;
  last = list->last;
  list->last = last->prev;
  if (!list->last) list->first = NULL;
  llist_remove(last);
  free(last);
  list->size--;
  
  return value;
}

void *list_last(struct list *list)
{
  if (!list->last) return NULL;
  return list->last->value;
}

int debug(int level, char* fmt, ...)
{

  int retval=0;
  va_list ap;

  if (level <= debug_level) {
    if (!debug_stream) debug_stream = stderr;
    va_start(ap, fmt); /* Initialize the va_list */
    retval = vfprintf(debug_stream, fmt, ap); /* Call vprintf */
    va_end(ap); /* Cleanup the va_list */
  }

  return retval;

}
