#include "TypeAnnotations.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace {

#define NULLABLE_ANN "nullable"

class NullnessAnnotator: public Annotator<NullnessAnnotator> {
public:
  NullnessAnnotator(CompilerInstance &ci, bool instrument)
      : Annotator(ci, instrument) {};

  bool nullable(const Expr *E) const {
    return AnnotationOf(E).equals(NULLABLE_ANN);
  }

  bool nullable(const QualType T) const {
    return AnnotationOf(T).equals(NULLABLE_ANN);
  }

  llvm::StringRef ImplicitAnnotation(const QualType QT) const {
    // Mark C++11 nullptr_t type as nullable.
    if (auto *RecTy = dyn_cast<RecordType>(QT.getCanonicalType())) {
      auto *D = RecTy->getDecl();
      if (D->getName() == "nullptr_t") {
        return NULLABLE_ANN;
      }
    }

    return StringRef();
  }

  // Check dereferencing and address-of expressions.
  llvm::StringRef VisitUnaryOperator(UnaryOperator *E) {
    switch (E->getOpcode()) {
    case UO_Deref:
      if (nullable(E->getSubExpr())) {
        unsigned did = Diags().getCustomDiagID(
          DiagnosticsEngine::Warning,
          "dereferencing nullable pointer"
        );
        Diags().Report(E->getLocStart(), did)
            << CharSourceRange(E->getSourceRange(), false);
      }
      break;
    case UO_AddrOf:
      // Address-of always non-null.
      break;
    default:
      // TODO check pointer arithmetic?
      break;
    }
    return StringRef();
  }

  // Mark null literals as null.
  llvm::StringRef VisitIntegerLiteral(IntegerLiteral *E) {
    if (E->getValue() == 0) {
      return NULLABLE_ANN;
    } else {
      return StringRef();
    }
  }

  // Subtyping judgment.
  bool Compatible(QualType LTy, QualType RTy) const {
    if (LTy->isPointerType()) {
      return nullable(LTy) || !nullable(RTy);
    } else {
      return true;
    }
  }

  void EmitIncompatibleError(clang::Stmt* S, QualType LTy,
                             QualType RTy) {
    // TODO would be nice if we could give more context about *which* non-null
    // pointer is the problem.
    unsigned did = Diags().getCustomDiagID(
      DiagnosticsEngine::Warning,
      "non-null pointer may become null"
    );
    Diags().Report(S->getLocStart(), did)
        << CharSourceRange(S->getSourceRange(), false);
  }
};

class NullnessAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    // Construct a type checker for our type system.
    return new TAConsumer<NullnessAnnotator>(CI, true);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {
    // No arguments.
    return true;
  }
};

}

static FrontendPluginRegistry::Add<NullnessAction>
X("nullness", "prevent null pointer dereferences");
