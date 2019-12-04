#include <stdio.h>

int *fuck() { return 0; }

int main(void) {
  int a = 10;
  int *p = a ? NULL : fuck();
}
