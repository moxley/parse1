#include <stdlib.h>
#include "filebuf.h"

int filebuf_init(struct t_filebuf *filebuf, FILE *in) {
  filebuf->in = in;
  filebuf->row = -1;
  filebuf->col = -1;
  filebuf->lines = malloc(LINES_ALLOC_SIZE + 1);
}

void filebuf_close(struct t_filebuf *filebuf) {
  free(filebuf->lines);
}

int filebuf_getline(struct t_filebuf *filebuf) {
  return 0;
}

int filebuf_getc(struct t_filebuf *filebuf) {
  return 0;
}
