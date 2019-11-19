int printf(const char *s, ...);
// cpp.c debug.c gen.c lex.c main.c map.c parse.c

static int sumi(int a, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, a);
  int acc = 0;
  for (int i = 0; i < a; ++i) acc += *(int *)__va_arg_gp(args);
  __builtin_va_end(args);
  return acc;
}

int main(void) { printf("%d\n", sumi(8, 0, 1, 2, 3, 4, 5, 6, 7)); }
