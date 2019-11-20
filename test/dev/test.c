#include <string.h>

int printf(const char* s, ...);

static char* arr[] = {"abc", (void*)0};
static char* arr[];

static void array_type_comp() {
  printf("%d\n", strcmp("abc", arr[0]));
  printf("%d\n", (int)arr[1]);
}

int main(void) { array_type_comp(); }
