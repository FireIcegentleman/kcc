int printf(const char *s, ...);

void fuckl(int a) { printf("%d\n", a); }

int main(void) { fuckl(_Alignof(char) - 2 > 0); }
