//
// Created by kaiser on 2019/11/4.
//

#ifndef KCC_INCLUDE_BUILTIN_H_
#define KCC_INCLUDE_BUILTIN_H_

typedef struct {
  unsigned int gp_offset;
  unsigned int fp_offset;
  void *overflow_arg_area;
  void *reg_save_area;
} __va_elem;

typedef __va_elem __builtin_va_list[1];

static void *__va_arg_gp(__va_elem *ap) {
  void *r = (char *)ap->reg_save_area + ap->gp_offset;
  ap->gp_offset += 8;
  return r;
}

static void *__va_arg_fp(__va_elem *ap) {
  void *r = (char *)ap->reg_save_area + ap->fp_offset;
  ap->fp_offset += 16;
  return r;
}

void __builtin_va_start(__builtin_va_list, int);
void __builtin_va_end(__builtin_va_list);

#define __builtin_va_copy(dest, src) ((dest)[0] = (src)[0])
#define __builtin_va_arg(ap, type) = \
    *(type *)(__builtin_reg_class(type) ? __va_arg_gp(ap) : __va_arg_fp(ap))

#endif  // KCC_INCLUDE_BUILTIN_H_
