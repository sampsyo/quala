// RUN: clang++ -fsyntax-only -Xclang -verify %s

#include <cstddef>

#define NULLABLE __attribute__((type_annotate("nullable")))

int main() {
  int *p;
  p = NULL;  // expected-warning {{may become null}}
  p = nullptr;  // expected-warning {{may become null}}
  return 0;
}
