//#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

// static void test_mix(char *p, ...) {
//  va_list ap;
//  va_start(ap, p);
//
//  printf("%f\n", va_arg(ap, double));
//  //printf("%d\n", va_arg(ap, int));
//
//  va_end(ap);
//}

int fuck() { return 0; }
int shit() { return 1; }

const char *a() { return "a"; }
const char *b() { return "b"; }
const char *c() { return "c"; }

int main(void) {
  // test_mix("abc", 2.6, 4, "d", 5);
  printf("%s", fuck() ? a() : (shit() ? b() : c()));
  printf(isatty(fileno(stdout)) ? "\e[32mOK\e[0m\n" : "OK\n");
}