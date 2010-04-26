#ifndef util_h
#define util_h

extern int debug_level;
extern FILE *debug_stream;

struct item {
  void *value;
  struct item *next;
  struct item *prev;
};

struct list {
  struct item *first;
  struct item *last;
  int size;
};

//struct item * llist_newitem(void *value);
//void llist_free(struct item *first);
//void llist_prepend(struct item *item, struct item *newitem);
//void llist_append(struct item *item, struct item *newitem);
//void llist_remove(struct item *item);

void list_init(struct list *list);
void list_empty(struct list *list);
int list_size(struct list *list);
int list_push(struct list *list, void *item);
void *list_pop(struct list *list);
void *list_top(struct list *list);
void *list_last(struct list *list);

int debug(int level, char* fmt, ...);

#endif
