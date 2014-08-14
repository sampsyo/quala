#include "TypeAnnotations.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace {

#define TAINTED_TYPE "tainted"

class TaintTyper : public Typer<TaintTyper> {
public:
  TaintTyper(CompilerInstance &ci, bool instrument)
      : Typer(ci, instrument) {};

  // Type rule for binary-operator expressions.
  llvm::StringRef VisitBinaryOperator(BinaryOperator *E) {
    // If either subexpression is tainted, so is this expression.
    if (TypeOf(E->getLHS()).equals(TAINTED_TYPE))
      return TAINTED_TYPE;
    if (TypeOf(E->getRHS()).equals(TAINTED_TYPE))
      return TAINTED_TYPE;

    // Otherwise, not tainted.
    return StringRef();
  }

  // And for unary-operator expressions.
  llvm::StringRef VisitUnaryOperator(UnaryOperator *E) {
    // Unary operator just has the type of its operand.
    return TypeOf(E->getSubExpr());
  }

  // Subtyping judgment.
  bool Compatible(StringRef LTy, StringRef RTy) {
    return LTy.equals(TAINTED_TYPE) || !RTy.equals(TAINTED_TYPE);
  }
};

class TaintTrackingAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    // Construct a type checker for our type system.
    return new TAConsumer<TaintTyper>(CI, true);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {
    // No arguments.
    return true;
  }
};

}

static FrontendPluginRegistry::Add<TaintTrackingAction>
X("taint-tracking", "information flow type system");
