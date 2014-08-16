#ifndef TYPE_ANNOTATIONS_H
#define TYPE_ANNOTATIONS_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Debug.h"
#include "clang/Frontend/MultiplexConsumer.h"

#include <algorithm>

#define DEBUG_TYPE "quala"

namespace clang {

// DANGEROUS HACK
// This is to gain access to the frontend's consumer order so we can change it
// (below).
class HackyMultiplexConsumer : public SemaConsumer {
public:
  std::vector<ASTConsumer*> Consumers;
};

template<typename ImplClass>
class Annotator : public StmtVisitor<ImplClass, llvm::StringRef> {
public:
  CompilerInstance &CI;
  ImplClass *impl;
  FunctionDecl *CurFunc;
  bool Instrument;

  Annotator(CompilerInstance &_ci, bool _instrument) :
    CI(_ci),
    impl(static_cast<ImplClass*>(this)),
    CurFunc(NULL),
    Instrument(_instrument)
  {};

  /*** ANNOTATION ASSIGNMENT HELPER ***/

  void AddAnnotation(Expr *E, StringRef A) const {
    // TODO check whether it already has the annotation & do nothing
    if (A.size()) {
      E->setType(CI.getASTContext().getAnnotatedType(E->getType(), A));
    }
  }

  void AddAnnotationOrImplicit(Expr *E, StringRef A) const {
    if (A.size()) {
      AddAnnotation(E, A);
    } else {
      AddAnnotation(E, impl->ImplicitAnnotation(E->getType()));
    }
  }

  /*** ANNOTATION LOOKUP HELPERS ***/

  // Override this to provide annotations on types regardless of where they
  // appear.
  llvm::StringRef ImplicitAnnotation(const QualType QT) const {
    return StringRef();
  }

  llvm::StringRef AnnotationOf(const Type *T) const {
    // TODO step through desugaring (typedefs, etc.)
    // TODO multiple annotations?
    if (auto *AT = llvm::dyn_cast<AnnotatedType>(T)) {
      return AT->getAnnotation();
    } else {
      return StringRef();
    }
  }

  llvm::StringRef AnnotationOf(QualType QT) const {
    // Try implicit annotation.
    StringRef ImplicitAnn = impl->ImplicitAnnotation(QT);
    if (ImplicitAnn.size()) {
      return ImplicitAnn;
    }

    // Look up the annotation.
    const Type *T = QT.getTypePtrOrNull();
    if (!T) {
      return StringRef();
    } else {
      return AnnotationOf(T);
    }
  }

  llvm::StringRef AnnotationOf(const Expr *E) const {
    if (!E) {
      return StringRef();
    } else {
      return AnnotationOf(E->getType());
    }
  }

  llvm::StringRef AnnotationOf(const ValueDecl *D) const {
    if (!D) {
      return StringRef();
    } else {
      return AnnotationOf(D->getType());
    }
  }

  /*** SUBTYPING (COMPATIBILITY) CHECKS ***/

  // For subclasses to override: determine compatibility of two types.
  bool Compatible(QualType LTy, QualType RTy) const {
    return true;
  }

  // Raise an "incompatible" error if the expressions are not compatible. Don't
  // override this.
  void AssertCompatible(Stmt *S, QualType LTy, QualType RTy) const {
    if (!impl->Compatible(LTy, RTy)) {
      impl->EmitIncompatibleError(S, LTy, RTy);
    }
  }

  DiagnosticsEngine &Diags() const {
    return CI.getDiagnostics();
  }

  void EmitIncompatibleError(clang::Stmt* S, QualType LTy,
                             QualType RTy) {
    unsigned did = Diags().getCustomDiagID(
      DiagnosticsEngine::Error,
      "%0 incompatible with %1"
    );
    StringRef LAnn = AnnotationOf(LTy);
    StringRef RAnn = AnnotationOf(RTy);
    Diags().Report(S->getLocStart(), did)
        << (RAnn.size() ? RAnn : "unannotated")
        << (LAnn.size() ? LAnn : "unannotated")
        << CharSourceRange(S->getSourceRange(), false);
  }

  /*** DEFAULT TYPING RULES ***/

  // Assignment compatibility.
  llvm::StringRef VisitBinAssign(BinaryOperator *E) {
    AssertCompatible(E, E->getLHS()->getType(), E->getRHS()->getType());
    return AnnotationOf(E->getLHS());
  }

  llvm::StringRef VisitCompoundAssignOperator(CompoundAssignOperator *E) {
    AssertCompatible(E, E->getLHS()->getType(), E->getRHS()->getType());
    return AnnotationOf(E->getLHS());
  }

  // Declaration initializers treated like assignments.
  llvm::StringRef VisitDeclStmt(DeclStmt *S) {
    for (auto i = S->decl_begin(); i != S->decl_end(); ++i) {
      auto *VD = dyn_cast<VarDecl>(*i);
      if (VD) {
        Expr *Init = VD->getInit();
        if (Init) {
          AssertCompatible(Init, VD->getType(), Init->getType());
        }
      }
    }
    return StringRef();
  }

  // Propagate types through implicit casts.
  llvm::StringRef VisitImplicitCastExpr(ImplicitCastExpr *E) {
    return AnnotationOf(E->getSubExpr());
  }

  llvm::StringRef VisitCallExpr(CallExpr *E) {
    // Check parameter types.
    FunctionDecl *D = E->getDirectCallee();
    if (D) {
      // Check parameter types.
      if (D->param_size() == E->getNumArgs()) {
        auto pi = D->param_begin();
        auto ai = E->arg_begin();
        for (; pi != D->param_end() && ai != E->arg_end(); ++pi, ++ai) {
          AssertCompatible(*ai, (*pi)->getType(), (*ai)->getType());
        }
      } else {
        // Parameter list length mismatch. Probably a varargs function. FIXME?
        DEBUG(llvm::errs() << "UNSOUND: varargs function\n");
      }

      return AnnotationOf(D->getReturnType());
    }

    // We couldn't determine which function is being called. Unsoundly, we
    // check nothing and return the null type. FIXME?
    DEBUG(llvm::errs() << "UNSOUND: indirect call\n");
    return StringRef();
  }

  llvm::StringRef VisitReturnStmt(ReturnStmt *S) {
    Expr *E = S->getRetValue();
    if (E) {
      assert(CurFunc && "return outside of function?");
      AssertCompatible(S, CurFunc->getReturnType(), E->getType());
    }
    return StringRef();
  }
};

// Hack to go bottom-up (postorder) on statements.
template<typename AnnotatorClass>
class TAVisitor : public RecursiveASTVisitor< TAVisitor<AnnotatorClass> > {
public:
  bool TraverseStmt(Stmt *S) {
    // Super traversal: visit children.
    RecursiveASTVisitor<TAVisitor>::TraverseStmt(S);

    // Now give type to parent.
    if (S) {
      StringRef ty = Annotator->Visit(S);
      if (auto *E = dyn_cast<Expr>(S)) {
        DEBUG(E->dump());
        DEBUG(llvm::errs() << "result type: " << ty << "\n");
        Annotator->AddAnnotationOrImplicit(E, ty);
      }
    }

    return true;
  }

  // Tell the typer which function it's inside.
  bool VisitFunctionDecl(FunctionDecl *D) {
    Annotator->CurFunc = D;
    return true;
  }

  // Disable "data recursion", which skips calls to Traverse*.
  bool shouldUseDataRecursionFor(Stmt *S) const { return false; }

  AnnotatorClass *Annotator;
};

template<typename AnnotatorClass>
class TAConsumer : public ASTConsumer {
public:
  CompilerInstance &CI;
  TAVisitor<AnnotatorClass> Visitor;
  AnnotatorClass Annotator;
  bool Instrument;

  TAConsumer(CompilerInstance &_ci, bool _instrument) :
    CI(_ci),
    Annotator(_ci, _instrument),
    Instrument(_instrument)
    {}

  virtual void Initialize(ASTContext &Context) {
    Visitor.Annotator = &Annotator;

    if (Instrument) {
      // DANGEROUS HACK
      // Change the order of the frontend's AST consumers. The
      // MultiplexConsumer contains references to (a) the "real" Clang
      // consumer, which does codegen and such, and (b) all the plugin
      // consumers. We need to move this plugin to the front of the list to
      // run before codegen. This is *certainly* unsupported and, to make it
      // even worse, requires hackery to access the private consumer vector
      // in MultiplexConsumer. Too bad.
      auto *mxc = cast<MultiplexConsumer>(&CI.getASTConsumer());
      auto *hmxc = (HackyMultiplexConsumer *)mxc;

      // Find this consumer in the list.
      auto it = std::find(hmxc->Consumers.begin(),
                          hmxc->Consumers.end(),
                          this);
      assert(*it == this && "consumer not found in multiplex list");

      // Move this from its current position to the first position.
      hmxc->Consumers.erase(it);
      hmxc->Consumers.insert(hmxc->Consumers.begin(), this);
    }
  }

  virtual void HandleTranslationUnit(ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
};

}

#undef DEBUG_TYPE

#endif
