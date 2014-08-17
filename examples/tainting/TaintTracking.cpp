#include "TypeAnnotations.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Basic/Builtins.h"
using namespace clang;

namespace {

#define TAINTED_ANN "tainted"
#define UNTAINTED_ANN "untainted"

class TaintAnnotator: public Annotator<TaintAnnotator> {
public:
  TaintAnnotator(CompilerInstance &ci, bool instrument)
      : Annotator(ci, instrument) {};

  // Check whether an expression or type has the "tainted" annotation.
  template <typename T>
  bool tainted(const T V) const {
    return AnnotationOf(V).equals(TAINTED_ANN);
  }

  // Type rule for binary-operator expressions.
  void VisitBinaryOperator(BinaryOperator *E) {
    // If either subexpression is tainted, so is this expression.
    if (tainted(E->getLHS()) || tainted(E->getRHS())) {
      AddAnnotation(E, TAINTED_ANN);
    }
  }

  // And for unary-operator expressions.
  void VisitUnaryOperator(UnaryOperator *E) {
    // Unary operator just has the type of its operand.
    AddAnnotation(E, AnnotationOf(E->getSubExpr()));
  }

  // Subtyping judgment.
  bool Compatible(QualType LTy, QualType RTy) {
    // Top-level annotation: disallow tainted-to-untainted flow.
    if (tainted(RTy) && !tainted(LTy)) {
      return false;
    }

    if (LTy->isPointerType() && RTy->isPointerType()) {
      // Unwrap pointer types to check that they have identical qualifiers in
      // their pointed-to types.
      ASTContext &Ctx = CI.getASTContext();
      while (Ctx.UnwrapSimilarPointerTypes(LTy, RTy)) {
        if (tainted(LTy) != tainted(RTy)) {
          return false;
        }
      }
      return true;  // Identical annotations.

    } else {
      // Non-pointer type. Above check suffices.
      return true;
    }
  }

  // Endorsements.
  void VisitCallExpr(CallExpr *E) {
    unsigned biid = E->getBuiltinCallee();
    if (biid == Builtin::BI__builtin_annotation) {
      auto *literal = cast<StringLiteral>(E->getArg(1));
      if (literal->getString() == "endorse") {
        // Most of the time, an untainted type just has no annotation
        // (i.e., the default). For endorsements, however, we need to mask any
        // potential "tainted" annotation anywhere in the hierarchy with
        // another annotation so that AnnotationOf returns this instead the
        // old type (in the case that the "tainted" is buried somewhere under
        // a typedef, for example).
        RemoveAnnotation(E);
        AddAnnotation(E, "untainted");
      }
    }
    Annotator<TaintAnnotator>::VisitCallExpr(E);
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
