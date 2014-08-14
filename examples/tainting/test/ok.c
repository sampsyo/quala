// RUN: clang -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#define TAINTED __attribute__((type_annotate("tainted")))

int main() {
  TAINTED int x = 2;
  TAINTED float y = (float)x;
  TAINTED int z;
  z = x + 5;
  return 0;
}
