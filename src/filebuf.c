#include <stdlib.h>
#include "filebuf.h"

char *filebuf_error_names[] = {
  "FB_ERR_NONE",
  "FB_ERR_INPUT"
};

int filebuf_init(struct t_filebuf *filebuf, FILE *in) {
  if (!in) {
    filebuf->error = FB_ERR_INPUT;
    return 1;
  }
  filebuf->error = FB_ERR_NONE;
  filebuf->in = in;
  filebuf->row = -1;
  filebuf->col = 0;
  filebuf->line[0] = '\0';
  filebuf->len = 0;
  return 0;
}

int filebuf_close(struct t_filebuf *filebuf) {
  if (filebuf->in) fclose(filebuf->in);
  return 0;
}

int filebuf_print(struct t_filebuf *filebuf) {
  printf("<#filebuf {error: %d, row: %d, col: %d, line: %s}>\n",
	 filebuf->error,
	 filebuf->row,
	 filebuf->col,
	 filebuf->line);
}

int filebuf_getline(struct t_filebuf *filebuf) {
  int i;

  for (i=0; 1; i++) {
    filebuf->c = fgetc(filebuf->in);
    if (i == MAX_LINE_LEN) {
      filebuf->error = FB_ERR_MAX_LINE_LEN;
      break;
    }
    else if (filebuf->c == EOF) {
      break;
    }
    else if (filebuf->c == '\n') {
      filebuf->line[i++] = filebuf->c;
      break;
    }
    else if (filebuf->c == '\r') {
      filebuf->line[i] = filebuf->c;
      filebuf->c = fgetc(filebuf->in);
      if (i+1 == MAX_LINE_LEN) {
	i++;
	if (filebuf->c != EOF) {
	  filebuf->error = FB_ERR_MAX_LINE_LEN;
	}
	break;
      }
      else if (filebuf->c != '\n') {
	i++;
	break;
      }
    }
    else {
      filebuf->line[i] = filebuf->c;
    }
  }
  filebuf->line[i] = '\0';
  filebuf->len = i;
  return filebuf->error;
}

int filebuf_getc(struct t_filebuf *filebuf) {
  if (filebuf->row != -1 && filebuf->len == 0) {
    return EOF;
  }
  else if (filebuf->row == -1 || filebuf->col+1 == filebuf->len) {
    if (filebuf_getline(filebuf)) return EOF;
    if (filebuf->len == 0) return EOF;
    filebuf->row++;
    filebuf->col = 0;
  }
  else {
    filebuf->col++;
  }
  /* printf("getc(). col: %d, c: %c\n", filebuf->col, filebuf->line[filebuf->col]); */
  return filebuf->line[filebuf->col];
}
