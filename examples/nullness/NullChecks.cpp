#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct NullChecks : public FunctionPass {
  static char ID;
  NullChecks() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F) {
    errs() << "Hello: ";
    errs().write_escaped(F.getName()) << '\n';
    return false;
  }
};

}

char NullChecks::ID = 0;
static RegisterPass<NullChecks> X("null-checks",
    "Insert checks for nullable pointers", false, false);
