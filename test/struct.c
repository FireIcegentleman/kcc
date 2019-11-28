// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "test.h"

static void t1() {
    struct { int a; } x;
    x.a = 61;
    expect(61, x.a);
}

static void t2() {
    struct { int a; int b; } x;
    x.a = 61;
    x.b = 2;
    expect(63, x.a + x.b);
}

static void t3() {
    struct { int a; struct { char b; int c; } y; } x;
    x.a = 61;
    x.y.b = 3;
    x.y.c = 3;
    expect(67, x.a + x.y.b + x.y.c);
}

static void t4() {
    struct tag4 { int a; struct { char b; int c; } y; } x;
    struct tag4 s;
    s.a = 61;
    s.y.b = 3;
    s.y.c = 3;
    expect(67, s.a + s.y.b + s.y.c);
}

static void t5() {
    struct tag5 { int a; } x;
    struct tag5 *p = &x;
    x.a = 68;
    expect(68, (*p).a);
}

static void t6() {
    struct tag6 { int a; } x;
    struct tag6 *p = &x;
    (*p).a = 69;
    expect(69, x.a);
}

static void t7() {
    struct tag7 { int a; int b; } x;
    struct tag7 *p = &x;
    x.b = 71;
    expect(71, (*p).b);
}

static void t8() {
    struct tag8 { int a; int b; } x;
    struct tag8 *p = &x;
    (*p).b = 72;
    expect(72, x.b);
}

static void t9() {
    struct tag9 { int a[3]; int b[3]; } x;
    x.a[0] = 73;
    expect(73, x.a[0]);
    x.b[1] = 74;
    expect(74, x.b[1]);
    expect(74, x.a[4]);
}

struct tag10 {
    int a;
    struct tag10a {
        char b;
        int c;
    } y;
} v10;
static void t10() {
  // FIXME
//    v10.a = 71;
//    v10.y.b = 3;
//    v10.y.c = 3;
//    expect(77, v10.a + v10.y.b + v10.y.c);
}

struct tag11 { int a; } v11;
static void t11() {
    struct tag11 *p = &v11;
    v11.a = 78;
    expect(78, (*p).a);
    expect(78, v11.a);
    expect(78, p->a);
    p->a = 79;
    expect(79, (*p).a);
    expect(79, v11.a);
    expect(79, p->a);
}

struct tag12 {
    int a;
    int b;
} x;
static void t12() {
    struct tag12 a[3];
    a[0].a = 83;
    expect(83, a[0].a);
    a[0].b = 84;
    expect(84, a[0].b);
    a[1].b = 85;
    expect(85, a[1].b);

    int *p = (int *)a;
    expect(85, p[3]);
}

static void t13() {
    struct { char c; } v = { 'a' };
    expect('a', v.c);
}

static void t14() {
    struct { int a[3]; } v = { { 1, 2, 3 } };
    expect(2, v.a[1]);
}

static void unnamed() {
    struct {
        union {
            struct { int x; int y; };
            struct { char c[8]; };
        };
    } v;
    v.x = 1;
    v.y = 7;

    expect(1, v.c[0]);
    expect(7, v.c[4]);
}

static void assign() {
    struct { int a, b, c; short d; char f; } v1, v2;
    v1.a = 3;
    v1.b = 5;
    v1.c = 7;
    v1.d = 9;
    v1.f = 11;
    v2 = v1;
    expect(3, v2.a);
    expect(5, v2.b);
    expect(7, v2.c);
    expect(9, v2.d);
    expect(11, v2.f);
}

static void arrow() {
    struct cell { int val; struct cell *next; };
    struct cell v1 = { 5, NULL };
    struct cell v2 = { 6, &v1 };
    struct cell v3 = { 7, &v2 };
    struct cell *p = &v3;
    expect(7, v3.val);
    expect(7, p->val);
    expect(6, p->next->val);
    expect(5, p->next->next->val);

    p->val = 10;
    p->next->val = 11;
    p->next->next->val = 12;
    expect(10, p->val);
    expect(11, p->next->val);
    expect(12, p->next->next->val);
}

static void address() {
    struct tag { int a; struct { int b; } y; } x = { 6, 7 };
    int *p1 = &x.a;
    int *p2 = &x.y.b;
    expect(6, *p1);
    expect(7, *p2);
    expect(6, *&x.a);
    expect(7, *&x.y.b);

    struct tag *xp = &x;
    int *p3 = &xp->a;
    int *p4 = &xp->y.b;
    expect(6, *p3);
    expect(7, *p4);
    expect(6, *&xp->a);
    expect(7, *&xp->y.b);
}

static void incomplete() {
    struct tag1;
    struct tag2 { struct tag1 *p; };
    struct tag1 { int x; };

    struct tag1 v1 = { 3 };
    struct tag2 v2 = { &v1 };
    expect(3, v2.p->x);
}

static void test_offsetof() {
    struct tag10 { int a, b; };
    expect(0, offsetof(struct tag10, a));
    expect(4, offsetof(struct tag10, b));
    int x[offsetof(struct tag10, b)];
    expect(4, sizeof(x) / sizeof(x[0]));

    expect(4, offsetof(struct { char a; struct { int b; }; }, b));
}

static void flexible_member() {
    struct { int a, b[]; } x;
    expect(4, sizeof(x));

    struct t { int a, b[]; };
}

static void empty_struct() {
    struct tag15 {};
    expect(0, sizeof(struct tag15));
    union tag16 {};
    expect(0, sizeof(union tag16));
}

static void incdec_struct() {
    struct incdec {
	int x, y;
    } a[] = { { 1, 2 }, { 3, 4 } }, *p = a;
    expect(1, p->x);
    expect(2, p->y);
    p++;
    expect(3, p->x);
    expect(4, p->y);
    p--;
    expect(1, p->x);
    expect(2, p->y);
    ++p;
    expect(3, p->x);
    expect(4, p->y);
    --p;
    expect(1, p->x);
    expect(2, p->y);
}

void test1()
{
  typedef struct {
    int a;
    char b;
    short c;
    long d;
  } foo_t;
  foo_t foo;
  memset((void*)&foo, 0, sizeof(foo_t));

  expect(foo.a, 0);
  expect(foo.b, 0);
  expect(foo.c, 0);
  expect(foo.d, 0);
}

void test2()
{
  typedef struct {
    char a;
    char b;
    char c;
    char d;
  } foo_t;
  foo_t foo;
  memset((void*)&foo, 1, sizeof foo);
  expect(foo.a, 1);
  expect(foo.b, 1);
  expect(foo.c, 1);
  expect(foo.d, 1);
}

void test3()
{
  typedef struct {
    char a;
    char b;
    char c;
    char d;
  } foo_t;

  foo_t foo = {1, 2, 3, 4};
  expect(foo.a, 1);
  expect(foo.b, 2);
  expect(foo.c, 3);
  expect(foo.d, 4);
}

void test4()
{
  typedef struct {
    union {
      char a;
      char b;
    };
    char c;
    char d;
  } foo_t;

  foo_t foo = {1, 2, 3};
  expect(foo.a, 1);
  expect(foo.b, 1);
  expect(foo.c, 2);
  expect(foo.d, 3);
}

void test5()
{
  typedef struct {
    union {
      struct {
        char b;
        char c;
      };
      short a;
    };
    char d;
    char e;
  } foo_t;

  foo_t foo = {1, 1, 2, 3};
  expect(foo.a, (1 << 8) + 1);
  expect(foo.b, 1);
  expect(foo.c, 1);
  expect(foo.d, 2);
  expect(foo.e, 3);
}

void test6()
{
  typedef union {
    char a;
    char b;
    char c;
    char d;
  } foo_t;

  foo_t foo = {1};
  expect(foo.a, 1);
  expect(foo.b, 1);
  expect(foo.c, 1);
  expect(foo.d, 1);
}

void test7()
{
  typedef struct tree tree_t;
  struct tree {
    int val;
    tree_t* left;
    tree_t* right;
  };

  tree_t* root = malloc(sizeof(tree_t));
  root->val = 5;
  root->left = (void*)10;
  root->right = (void*)20;
  expect(5, root->val);
  expect((void*)10, root->left);
  expect((void*)20, root->right);
}

void test8()
{
  typedef struct {
    struct {
      int a;
      char b;
    };
    char c;
  } foo_t;
  expect(12, sizeof(foo_t));
}

void test9()
{
  typedef struct {
    union {
      struct {
        char a;
        char b;
      };
      char c;
      char d;
    };
    char e;
  } foo_t;

  foo_t foo = {1, 2, 5, .d=3, 4};
  expect(3, foo.a);
  expect(3, foo.c);
  expect(3, foo.d);
  expect(4, foo.e);
}

void testmain() {
    print("struct");
    t1();
    t2();
    t3();
    t4();
    t5();
    t6();
    t7();
    t8();
    t9();
    t10();
    t11();
    t12();
    t13();
    t14();
    unnamed();
    assign();
    arrow();
    incomplete();
    test_offsetof();
    flexible_member();
    empty_struct();

    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
}
