// RUN: clang -fsyntax-only -Xclang -verify %s

#define TAINTED __attribute__((type_annotate("tainted")))

int main() {
  // Simplest possible flow violation.
  TAINTED int x;
  x = 5;
  int y;
  y = x; // expected-error {{incompatible}}

  // Opposite direction of flow is allowed.
  x = y;

  // Binary operator.
  y = x + 3; // expected-error {{incompatible}}

  // Unary operator.
  y = -x; // expected-error {{incompatible}}

  // Implicit casts (coercions).
  float z;
  z = x; // expected-error {{incompatible}}

  // Initializers are treated like assignments.
  int a = x * 9; // expected-error {{incompatible}}

  return 0;
}
