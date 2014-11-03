#include "AnnotationInfo.h"

using namespace llvm;

AnnotationInfo::AnnotationInfo() : ModulePass(ID) {}

bool AnnotationInfo::runOnModule(llvm::Module &M) {
  return false;
}

char AnnotationInfo::ID = 0;
static RegisterPass<AnnotationInfo> X("annotation-info",
                                      "gather type annotations",
                                      false,
                                      true);
