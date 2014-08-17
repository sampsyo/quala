// RUN: clang++ -fsyntax-only -Xclang -verify %s

#define TAINTED __attribute__((type_annotate("tainted")))

int main() {
  TAINTED int x;
  int y;

  x = y;
  TAINTED int &r = y; // expected-error {{incompatible}}

  return 0;
}
