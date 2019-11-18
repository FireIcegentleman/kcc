int printf(const char *s, ...);

int main(void) {
  int acc = 0;
  goto x;
  acc = 5;
x:
  printf("%d\n", acc);
}
