typedef struct {
  unsigned int gp_offset;
  unsigned int fp_offset;
  void *overflow_arg_area;
  void *reg_save_area;
} __va_elem;
typedef __va_elem __builtin_va_list[1];
void __builtin_va_start(__builtin_va_list, int);
void __builtin_va_end(__builtin_va_list);
# 1 "test.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 336 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "test.c" 2
# 27 "test.c"
int main(void) { int a[5] = {L"fu"}; }
