// RUN: clang -fsyntax-only -Xclang -verify %s

#define TAINTED __attribute__((type_annotate("tainted")))

int main() {
  TAINTED int x = 5;
  int y = 6;

  if (x) {}  // expected-error {{condition}}
  if (y) {}
  if (x == 2) {}  // expected-error {{condition}}
  if (y == 2) {}
  for (; x != 10;) {}  // expected-error {{condition}}
  for (; y != 10;) {}
  while (x) {}  // expected-error {{condition}}
  while (y) {}
  do {} while (x);  // expected-error {{condition}}
  do {} while (y);
  y = x ? 1 : 0;  // expected-error {{condition}}
  y = y ? 1 : 0;

  return 0;
}
