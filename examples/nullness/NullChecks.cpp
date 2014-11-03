#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

#include "AnnotationInfo.h"

using namespace llvm;

namespace {

struct NullChecks : public FunctionPass {
  static char ID;
  NullChecks() : FunctionPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    Info.addRequired<AnnotationInfo>();
  }

  virtual bool runOnFunction(Function &F) {
    AnnotationInfo &AI = getAnalysis<AnnotationInfo>();
    for (auto &BB : F) {
      for (auto &I : BB) {
        Value *Ptr = nullptr;
        if (auto *LI = dyn_cast<LoadInst>(&I)) {
          Ptr = LI->getPointerOperand();
        } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
          Ptr = SI->getPointerOperand();
        }

        // Dereferencing a pointer (either for a load or a store). Insert a
        // check if the pointer is nullable.
        if (Ptr) {
          if (AI.hasAnnotation(Ptr, "nullable")) {
            Ptr->dump();
          }
        }
      }
    }
    return false;
  }
};

}

char NullChecks::ID = 0;

// http://homes.cs.washington.edu/~asampson/blog/clangpass.html
static void registerPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new NullChecks());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerPass);
