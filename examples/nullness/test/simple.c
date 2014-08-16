// RUN: clang -fsyntax-only -Xclang -verify %s

#define NULLABLE __attribute__((type_annotate("nullable")))

int main() {
  int *a;
  int * NULLABLE b;

  a = 0;  // expected-warning {{may become null}}
  b = 0;
  a = b;  // expected-warning {{may become null}}
  b = a;
  *a = 1;
  *b = 1;  // expected-warning {{dereferencing nullable}}

  // Here's something that non-type-based checkers can't do.
  int * NULLABLE *c;
  int ** NULLABLE d;
  c = 0;  // expected-warning {{may become null}}
  d = 0;
  *c = 0;
  *d = 0;  // expected-warning {{may become null}} \
      expected-warning {{dereferencing nullable}}

  return 0;
}
