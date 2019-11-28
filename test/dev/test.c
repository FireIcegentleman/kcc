#include <stdio.h>
struct tag10 {
  int a;
  struct tag10a {
    char b;
    int c;
  } y;
} v10;
static void t10() {
  v10.a = 71;
  v10.y.b = 3;
  v10.y.c = 3;
}
int main(void) {
  t10();
  printf("%d\n", v10.a);
  printf("%d\n", v10.y.b);
  printf("%d\n", v10.y.c);
}
