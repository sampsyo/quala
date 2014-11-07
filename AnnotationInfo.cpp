#include "AnnotationInfo.h"
#include <llvm/IR/Constants.h>

using namespace llvm;

AnnotationInfo::AnnotationInfo() : ModulePass(ID) {}

bool AnnotationInfo::runOnModule(Module &M) {
  return false;
}

bool AnnotationInfo::hasAnnotation(Value *V, StringRef Ann, uint8_t level) {
  // Check instruction metadata.
  if (auto *I = dyn_cast<Instruction>(V)) {
    MDNode *MD = I->getMetadata("tyann");
    if (MD) {
      auto *MDS = cast<MDString>(MD->getOperand(0));
      if (MDS->getString().equals(Ann)) {
        auto *CI = cast<ConstantInt>(MD->getOperand(1));
        if (CI->getValue() == level) {
          return true;
        } else {
          return false;
        }
      }
    }
  }

  // TODO: Check for annotations on globals, parameters.

  return false;
}

char AnnotationInfo::ID = 0;
static RegisterPass<AnnotationInfo> X("annotation-info",
                                      "gather type annotations",
                                      false,
                                      true);
