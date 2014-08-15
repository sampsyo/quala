#include "TypeAnnotations.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace {

#define TAINTED_ANN "tainted"

class TaintAnnotator: public Annotator<TaintAnnotator> {
public:
  TaintAnnotator(CompilerInstance &ci, bool instrument)
      : Annotator(ci, instrument) {};

  bool tainted(const Expr *E) const {
    return AnnotationOf(E).equals(TAINTED_ANN);
  }

  bool tainted(const QualType T) const {
    return AnnotationOf(T).equals(TAINTED_ANN);
  }

  // Type rule for binary-operator expressions.
  llvm::StringRef VisitBinaryOperator(BinaryOperator *E) {
    // If either subexpression is tainted, so is this expression.
    if (tainted(E->getLHS()) || tainted(E->getRHS()))
      return TAINTED_ANN;

    // Otherwise, not tainted.
    return StringRef();
  }

  // And for unary-operator expressions.
  llvm::StringRef VisitUnaryOperator(UnaryOperator *E) {
    // Unary operator just has the type of its operand.
    return AnnotationOf(E->getSubExpr());
  }

  // Subtyping judgment.
  bool Compatible(QualType LTy, QualType RTy) {
    return tainted(LTy) || !tainted(RTy);
  }
};

class TaintTrackingAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    // Construct a type checker for our type system.
    return new TAConsumer<TaintAnnotator>(CI, true);
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
