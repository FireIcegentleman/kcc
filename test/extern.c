// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include "test.h"

extern int externvar1;
int extern externvar2;

int a = 3;
void test() {
  extern int a;
  expect(a, 3);
  {
    int a = 4;
    expect(a, 4);
  }
  {
    extern int a;
    expect(a, 3);
    a = 5;
    {
      extern int a;
      expect(a, 5);
    }
  }
}

void testmain() {
    print("extern");
    expect(98, externvar1);
    expect(99, externvar2);
    test();
}
