int printf(const char *s, ...);

int main(void) {
  int a = 10;
  printf("%d\n", a);
  printf("%d\n", a++);
  printf("%d\n", ++a);
}
