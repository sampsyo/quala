// RUN: clang -Xclang -verify -emit-llvm -S -o - %s | FileCheck %s
// expected-no-diagnostics

#define TAINTED __attribute__((type_annotate("tainted")))

int main() {
  // CHECK: %x = alloca i32, align 4, !tyann [[TAINTED:![0-9]+]]
  // CHECK: %y = alloca i32, align 4
  // CHECK: %z = alloca i32, align 4, !tyann [[TAINTED]]

  // CHECK: store i32 2, i32* %x, align 4, !tyann [[TAINTED]]
  TAINTED int x = 2;

  // CHECK: store i32 3, i32* %y, align 4
  int y = 3;

  // CHECK: %add = add nsw i32 %{{[0-9]+}}, %{{[0-9]+}}, !tyann [[TAINTED]]
  TAINTED int z;
  z = x + y;

  return 0;
}

// CHECK: [[TAINTED]] = metadata !{metadata !"tainted", i8 0}
