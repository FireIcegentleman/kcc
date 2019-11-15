int printf(const char *s, ...);

int main(void) {
  int a = 10;
  int *p = &a;
  printf("%d\n", a);
  *p = 33;
  printf("%d\n", a);
  a = 3;
  printf("%d\n", a);
  ++a;
  --a;
}