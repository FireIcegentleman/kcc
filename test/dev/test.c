int printf(const char *s, ...);

struct A {
  int a;
  struct {
    int b[3];
    struct {
      int c[3];
    };
  };
};

int main(void) {
  struct A aaa;
  //aaa.c[1] = 2;
}
