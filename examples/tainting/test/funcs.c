// RUN: clang -fsyntax-only -Xclang -verify %s

#define TAINTED __attribute__((type_annotate("tainted")))

// Parameter is tainted (see below for calls).
void f(TAINTED int p) {
}

// Return value is tainted (ditto).
TAINTED int g() {
  return 2;
}

// Return value is checked.
int h(TAINTED int p) {
  return p; // expected-error {{incompatible}}
}

// Opposite return value flow is OK.
TAINTED int i(int p) {
  return p;
}

int main() {
  TAINTED int x = 5;
  int y = 10;

  // Flow into parameter.
  f(x);
  f(y);
  i(x); // expected-error {{incompatible}}
  i(y);

  // Flow out of return value.
  y = g(); // expected-error {{incompatible}}
  x = g();
  y = h(1);
  x = h(1);

  return 0;
}
