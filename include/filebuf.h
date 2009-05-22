#ifndef filebuf_h
#define filebuf_h

#include <stdio.h>

#define LINES_ALLOC_SIZE 1000

struct t_filebuf {
  FILE *in;
  int row;
  int col;
  char *line;
};

int filebuf_init(struct t_filebuf *filebuf, FILE *in);
int filebuf_close(struct t_filebuf *filebuf);
int filebuf_getc(struct t_filebuf *filebuf);
int filebuf_getline(struct t_filebuf *filebuf);

#endif
