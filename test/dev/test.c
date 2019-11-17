#include <float.h>
#include <stdarg.h>
#include <stdio.h>

char *fmt(char *fmt, ...) {
  static char buf[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  return buf;
}

char *fmtdbl(double x) { return fmt("%a", x); }

int main(void) {
  // 1.19209290e-7F
  // 0x1p-23
  printf("%s\n", fmtdbl(FLT_EPSILON));
}