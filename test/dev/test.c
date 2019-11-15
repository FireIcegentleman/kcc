int printf(const char *s, ...);

struct A {
  int a;
  struct {
    int b;
  };
};

int main(void) {
  struct A aaa;
  aaa.b = 2;
  printf("%d\n", aaa.b);
}
