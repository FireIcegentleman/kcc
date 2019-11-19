int printf(const char *s, ...);

int main(void) {
  char c;
  switch (c) {
    case '"':
      return "\\\"";
    case '\\':
      return "\\\\";
    case '\b':
      return "\\b";
    case '\f':
      return "\\f";
    case '\n':
      return "\\n";
    case '\r':
      return "\\r";
    case '\t':
      return "\\t";
  }
}
