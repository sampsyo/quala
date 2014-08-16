// RUN: clang -fsyntax-only -Xclang -verify %s

#define TAINTED __attribute__((type_annotate("tainted")))
#define ENDORSE(e) __builtin_annotation((e), "endorse")

int main() {
  // Simplest possible flow violation.
  TAINTED int x;
  x = 5;
  int y;
  y = x; // expected-error {{incompatible}}
  y = ENDORSE(x);

  return 0;
}
