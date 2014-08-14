// RUN: clang -fsyntax-only -Xclang -verify %s

#define NULLABLE __attribute__((type_annotate("nullable")))

int main() {
  int *a;
  int * NULLABLE b;

  a = 0;  // expected-warning {{may become null}}
  b = 0;
  a = b;  // expected-warning {{may become null}}
  b = a;

  return 0;
}
