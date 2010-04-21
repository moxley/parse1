#ifndef util_h
#define util_h

struct item {
  void *value;
  struct item *next;
  struct item *prev;
};

struct item * llist_newitem(void *value);
void llist_free(struct item *first);
void llist_prepend(struct item *item, struct item *newitem);
void llist_append(struct item *item, struct item *newitem);
void llist_remove(struct item *item);

struct stack {
  struct item *bottom;
  struct item *top;
  int size;
};

void stack_init(struct stack *stack);
void stack_empty(struct stack *stack);
int stack_push(struct stack *stack, void *item);
void *stack_pop(struct stack *stack);
void *stack_top(struct stack *stack);

#endif
