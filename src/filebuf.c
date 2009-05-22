#include <stdlib.h>
#include "filebuf.h"

int filebuf_init(struct t_filebuf *filebuf, FILE *in) {
  filebuf->in = in;
  filebuf->row = -1;
  filebuf->col = -1;
  filebuf->line = (char *) 0;
}

int filebuf_close(struct t_filebuf *filebuf) {
  free(filebuf->line);
  return 0;
}

int filebuf_getline(struct t_filebuf *filebuf) {
  return 0;
}

int filebuf_getc(struct t_filebuf *filebuf) {
  return 0;
}
