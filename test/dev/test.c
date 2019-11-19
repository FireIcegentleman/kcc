#include <stdarg.h>
#include <stdio.h>
// cpp.c debug.c gen.c lex.c main.c map.c parse.c

void sumi(int a, ...) {
  va_list args;
  va_start(args, a);
  printf("%d\n", va_arg(args, int));
  va_end(args);
}

int main(void) { sumi(8, 0, 1, 2, 3, 4, 5, 6, 7); }
