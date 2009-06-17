#ifndef filebuf_h
#define filebuf_h

#include <stdio.h>

#define MAX_LINE_LEN 1000

#define FB_ERR_NONE 0
#define FB_ERR_INPUT 1
#define FB_ERR_MAX_LINE_LEN 2

extern char *filebuf_error_names[];

struct t_filebuf {
  FILE *in;
  int row;
  int col;
  int c;
  char line[MAX_LINE_LEN+1];
  int len;
  int error;
};

int filebuf_init(struct t_filebuf *filebuf, FILE *in);
int filebuf_close(struct t_filebuf *filebuf);
int filebuf_print(struct t_filebuf *filebuf);
int filebuf_getc(struct t_filebuf *filebuf);
int filebuf_getline(struct t_filebuf *filebuf);

#endif
