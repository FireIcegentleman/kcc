#include <stdarg.h>
#include <stdio.h>
// cpp.c debug.c gen.c lex.c main.c map.c parse.c

static int sumi(int a, ...) {
  va_list args;
  va_start(args, a);
  int acc = 0;
  for (int i = 0; i < a; ++i) acc += va_arg(args, int);
  va_end(args);
  return acc;
}

int main(void) { printf("%d\n", sumi(8, 0, 1, 2, 3, 4, 5, 6, 7)); }
