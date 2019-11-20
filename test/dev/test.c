int printf(const char* s, ...);

static char* arr[] = {"abc", (void*)0};
static char* arr[];

int main(void) { printf("%d\n", (int)arr[1]); }
