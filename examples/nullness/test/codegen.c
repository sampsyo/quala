// RUN: clang -Xclang -verify -emit-llvm -S -o - %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

#define NULLABLE __attribute__((type_annotate("nullable")))

int qualaHandleNull() {
  fprintf(stderr, "saved from a null dereference!\n");
  exit(1);
}

int main(int argc, char **argv) {
  // CHECK: store i32* null, i32** %foo, align {{[0-9]+}}, !tyann [[NULLABLE:![0-9]+]]
  int * NULLABLE foo = 0;
  // CHECK: call void @qualaNullCheck(i1 %isnull{{[0-9]*}})
  return *foo;  // expected-warning {{dereferencing nullable}}
}

// CHECK: [[NULLABLE]] = metadata !{metadata !"nullable", i8 0}
