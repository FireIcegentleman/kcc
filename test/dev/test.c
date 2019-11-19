int printf(const char *s, ...);

int main(void) {
  struct {
    union {
      struct {
        int x;
        int y;
      };
      struct {
        char c[8];
      };
    };
  } v;
  v.x = 1;
  v.y = 7;

  printf("%d\n", v.x);
  printf("%d\n", v.y);
}
