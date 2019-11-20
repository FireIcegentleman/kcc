#include <stdio.h>

int main(void) {
  long acc = 0;
  goto x;
  acc = 5;
x:
  printf("%ld\n", acc);
}
