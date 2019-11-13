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
# 23 "test.c"
# 1 "/usr/include/uchar.h" 1 3
# 26 "/usr/include/uchar.h" 3
# 1 "/usr/include/features.h" 1 3
# 428 "/usr/include/features.h" 3
# 1 "/usr/include/stdc-predef.h" 1 3
# 429 "/usr/include/features.h" 2 3
# 450 "/usr/include/features.h" 3
# 1 "/usr/include/sys/cdefs.h" 1 3
# 460 "/usr/include/sys/cdefs.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 461 "/usr/include/sys/cdefs.h" 2 3
# 1 "/usr/include/bits/long-double.h" 1 3
# 462 "/usr/include/sys/cdefs.h" 2 3
# 451 "/usr/include/features.h" 2 3
# 474 "/usr/include/features.h" 3
# 1 "/usr/include/gnu/stubs.h" 1 3
# 10 "/usr/include/gnu/stubs.h" 3
# 1 "/usr/include/gnu/stubs-64.h" 1 3
# 11 "/usr/include/gnu/stubs.h" 2 3
# 475 "/usr/include/features.h" 2 3
# 27 "/usr/include/uchar.h" 2 3


# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/include/stddef.h" 1 3
# 209 "/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/include/stddef.h" 3
typedef long unsigned int size_t;
# 30 "/usr/include/uchar.h" 2 3

# 1 "/usr/include/bits/types.h" 1 3
# 27 "/usr/include/bits/types.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 28 "/usr/include/bits/types.h" 2 3
# 1 "/usr/include/bits/timesize.h" 1 3
# 29 "/usr/include/bits/types.h" 2 3


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/bits/types.h" 3
# 1 "/usr/include/bits/typesizes.h" 1 3
# 142 "/usr/include/bits/types.h" 2 3
# 1 "/usr/include/bits/time64.h" 1 3
# 143 "/usr/include/bits/types.h" 2 3


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 32 "/usr/include/uchar.h" 2 3
# 1 "/usr/include/bits/types/mbstate_t.h" 1 3



# 1 "/usr/include/bits/types/__mbstate_t.h" 1 3
# 13 "/usr/include/bits/types/__mbstate_t.h" 3
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 5 "/usr/include/bits/types/mbstate_t.h" 2 3

typedef __mbstate_t mbstate_t;
# 33 "/usr/include/uchar.h" 2 3



typedef __uint_least16_t char16_t;
typedef __uint_least32_t char32_t;







extern size_t mbrtoc16 (char16_t *__restrict __pc16,
   const char *__restrict __s, size_t __n,
   mbstate_t *__restrict __p) __attribute__ ((__nothrow__ ));


extern size_t c16rtomb (char *__restrict __s, char16_t __c16,
   mbstate_t *__restrict __ps) __attribute__ ((__nothrow__ ));





extern size_t mbrtoc32 (char32_t *__restrict __pc32,
   const char *__restrict __s, size_t __n,
   mbstate_t *__restrict __p) __attribute__ ((__nothrow__ ));


extern size_t c32rtomb (char *__restrict __s, char32_t __c32,
   mbstate_t *__restrict __ps) __attribute__ ((__nothrow__ ));
# 24 "test.c" 2



int main(void) { char32_t 你好[5] = {L"fu"}; }
