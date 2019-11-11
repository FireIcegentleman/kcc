# 1 "test/dev/test.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 345 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "test/dev/test.c" 2
# 28 "test/dev/test.c"
int printf(const char *s,...);

int main(void) {

  printf("%s", __func__);
}
