# 1 "include/builtin.h"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 345 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "include/builtin.h" 2







typedef struct {
  unsigned int gp_offset;
  unsigned int fp_offset;
  void *overflow_arg_area;
  void *reg_save_area;
} __va_elem;

typedef __va_elem __builtin_va_list[1];
# 29 "include/builtin.h"
void __builtin_va_start(__builtin_va_list, int);
void __builtin_va_end(__builtin_va_list);
# 1 "test/dev/test.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 345 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "test/dev/test.c" 2
# 29 "test/dev/test.c"
int printf(const char *s, ...);
int main(void) {
  int a;
  a = 10;

  printf("%d\n", a ? 4 : 8);
}
