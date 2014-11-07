#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/LegacyPassManager.h"

struct AnnotationInfo : public llvm::ModulePass {
  static char ID;
  AnnotationInfo();
  virtual bool runOnModule(llvm::Module &M);
  bool hasAnnotation(llvm::Value *V, llvm::StringRef Ann, uint8_t level=0);
};
