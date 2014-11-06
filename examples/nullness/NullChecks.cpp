#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/IRBuilder.h"
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
    bool modified = false;

    for (auto &BB : F) {
      for (auto &I : BB) {
        // Is this a load or store? Get the address.
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
            addCheck(*Ptr, I);
            modified = true;
          }
        }
      }
    }

    return modified;
  }

  Function &getCheckFunc(Module &M) {
    LLVMContext &Ctx = M.getContext();

    // Get or add the declaration.
    Constant *C = M.getOrInsertFunction("qualaNullCheck",
        Type::getVoidTy(Ctx), Type::getInt1Ty(Ctx), NULL);
    auto *F = cast<Function>(C);

    // If the function does not have a body yet, write it.
    if (F->empty()) {
      BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", F);
      BasicBlock *Success = BasicBlock::Create(Ctx, "success", F);
      BasicBlock *Failure = BasicBlock::Create(Ctx, "failure", F);

      // Check block.
      Value *isnull = F->arg_begin();
      IRBuilder<> Bld(Entry);
      Bld.CreateCondBr(isnull, Failure, Success);

      // Success (non-null) block.
      Bld.SetInsertPoint(Success);
      Bld.CreateRetVoid();

      // Failure (null) block. If there's a funciton in the module to handle
      // this, call it. Otherwise, call exit(3).
      Bld.SetInsertPoint(Failure);
      if (Function *Handler = M.getFunction("qualaHandleNull")) {
        // Call the user's handler.
        Bld.CreateCall(Handler);
        Bld.CreateRetVoid();
      } else {
        // Call exit(3).
        AttributeSet Attrs;
        Attrs.addAttribute(Ctx, AttributeSet::FunctionIndex,
            Attribute::NoReturn);
        Constant *Exit = M.getOrInsertFunction("exit", Attrs,
            Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), NULL);
        Bld.CreateCall(Exit, Bld.getInt32(1));
        Bld.CreateUnreachable();
      }
    }

    return *F;
  }

  // Insert a null check for the given pointer value just before the
  // instruction.
  void addCheck(Value &Ptr, Instruction &I) {
    IRBuilder<> Bld(&I);
    Value *isnull = Bld.CreateIsNull(&Ptr, "isnull");
    Module *M = I.getParent()->getParent()->getParent();
    Bld.CreateCall(&(getCheckFunc(*M)), isnull);
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
