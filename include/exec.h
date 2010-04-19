#ifndef exec_h
#define exec_h

#include "parser.h"
#include "util.h"

extern struct item *firstfunc;
extern struct item *lastfunc;

struct t_expr * exec_stmt(struct t_expr *stmt);
struct t_expr * exec_eval(struct t_expr *expr, struct t_expr *value);

void exec_addfunc(struct t_func *func);
struct t_func * exec_funcbyname(char *name);
struct t_expr * exec_invoke(struct t_func *func, struct t_fcall *call);

int exec_run(struct t_scanner *scanner);
int exec_func(struct t_scanner *scanner);
int parser_parse_func(struct t_scanner *scanner, struct t_func *func); 
int parser_parse_func_args(struct t_scanner *scanner, struct t_func *func);

#endif
