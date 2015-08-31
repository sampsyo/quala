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

  // Check for NULLABLE annotation.
  template <typename T>
  bool nullable(const T V) const {
    return AnnotationOf(V).equals(NULLABLE_ANN);
  }

  // Check dereferencing and address-of expressions.
  void VisitUnaryOperator(UnaryOperator *E) {
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
  }

  // Mark null literals as null.
  void VisitIntegerLiteral(IntegerLiteral *E) {
    if (E->getValue() == 0) {
      AddAnnotation(E, NULLABLE_ANN);
    }
  }

  // The GNU C++ NULL expression.
  void VisitGNUNullExpr(GNUNullExpr *E) {
    AddAnnotation(E, NULLABLE_ANN);
  }

  // C++11 nullptr_t conversions.
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *E) {
    auto *D = E->getRecordDecl();
    if (D->getName() == "nullptr_t") {
      if (auto *Conv = dyn_cast<CXXConversionDecl>(E->getMethodDecl())) {
        if (Conv->getConversionType()->isPointerType()) {
          // An `operator T*`.
          AddAnnotation(E, NULLABLE_ANN);
        }
      }
    }
    Annotator<NullnessAnnotator>::VisitCXXMemberCallExpr(E);
  }

  // Subtyping judgment.
  bool Compatible(QualType LTy, QualType RTy) const {
    if (LTy->isPointerType()) {
      return nullable(LTy) || !nullable(RTy);
    }
    return CheckPointerInvariance(LTy, RTy);
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
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) {
    // Construct a type checker for our type system.
    return llvm::make_unique< TAConsumer<NullnessAnnotator> >(CI, true);
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
