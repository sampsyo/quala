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
class Annotator : public StmtVisitor<ImplClass> {
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

  /*** ANNOTATION ASSIGNMENT HELPERS ***/

  void AddAnnotation(Expr *E, StringRef A) const {
    // TODO check whether it already has the annotation & do nothing
    if (A.size()) {
      E->setType(CI.getASTContext().getAnnotatedType(E->getType(), A));
    }
  }

  // Remove an annotated type *at the outermost level of the type tree*. For
  // example, this will not remove annotations under typedefs (which seems
  // impossible).
  void RemoveAnnotation(Expr *E) const {
    // TODO remove a specific annotation? or all, if multiple?
    // Look for an AnnotatedType in the desugaring chain.
    QualType T = E->getType();
    if (auto *AT = dyn_cast<AnnotatedType>(T)) {
      E->setType(AT->getBaseType());
    }
  }

  /*** ANNOTATION LOOKUP HELPERS ***/

  // Override this to provide annotations on types regardless of where they
  // appear.
  llvm::StringRef ImplicitAnnotation(const QualType QT) const {
    return StringRef();
  }

  llvm::StringRef AnnotationOf(const Type *T) const {
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
    auto &Ctx = CI.getASTContext();
    for (;;) {
      const Type *T = QT.getTypePtrOrNull();
      if (!T) {
        return StringRef();
      }
      StringRef Ann = AnnotationOf(T);
      if (Ann.size())
        return Ann;

      // Try stripping away one level of sugar.
      QualType DT = QT.getSingleStepDesugaredType(Ctx);
      if (DT == QT) {
        break;
      } else {
        QT = DT;
      }
    }

    // No annotations found in desugaring sequence.
    return StringRef();
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

  bool SamePointerTypeAnnotations(QualType T1, QualType T2,
                                  bool outer=true) const {
    // Optionally check the annotation on the types themselves.
    if (outer && AnnotationOf(T1) != AnnotationOf(T2))
      return false;

    // Unwrap pointer types to check that they have identical qualifiers in
    // their pointed-to types.
    ASTContext &Ctx = CI.getASTContext();
    while (Ctx.UnwrapSimilarPointerTypes(T1, T2)) {
      if (AnnotationOf(T1) != AnnotationOf(T2)) {
        return false;
      }
    }
    return true;  // Identical annotations.
  }

  // Utility for checking compatibility when pointer types are invariant
  // (i.e., if you are sane). Returns true for non-pointer types. For pointer
  // (and reference types), ensures that every annotation *below the top
  // layer* is identical between the two types. You are supposed to check the
  // top layer yourself, since those annotations apply to value types (i.e.,
  // the pointers themselves).
  bool CheckPointerInvariance(QualType LTy, QualType RTy) const {
    if (LTy->isReferenceType() && !RTy->isReferenceType()) {
      // Reference binding. Strip off the LHS's reference and compare from
      // there.
      return SamePointerTypeAnnotations(LTy->getPointeeType(), RTy, true);
    } else if (LTy->isPointerType() && RTy->isPointerType()) {
      // Must have identical annotations (either direction of flow is an
      // error). Enforce comparison after the top level.
      return SamePointerTypeAnnotations(LTy, RTy, false);
    } else {
      // Non-pointer type. Above check suffices.
      return true;
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
  void VisitBinAssign(BinaryOperator *E) {
    AssertCompatible(E, E->getLHS()->getType(), E->getRHS()->getType());
    AddAnnotation(E, AnnotationOf(E->getLHS()));
  }

  void VisitCompoundAssignOperator(CompoundAssignOperator *E) {
    AssertCompatible(E, E->getLHS()->getType(), E->getRHS()->getType());
    AddAnnotation(E, AnnotationOf(E->getLHS()));
  }

  // Declaration initializers treated like assignments.
  void VisitDeclStmt(DeclStmt *S) {
    for (auto i = S->decl_begin(); i != S->decl_end(); ++i) {
      auto *VD = dyn_cast<VarDecl>(*i);
      if (VD) {
        Expr *Init = VD->getInit();
        if (Init) {
          AssertCompatible(Init, VD->getType(), Init->getType());
        }
      }
    }
  }

  // Propagate types through implicit casts and other "invisible" expressions.
  void VisitImplicitCastExpr(ImplicitCastExpr *E) {
    AddAnnotation(E, AnnotationOf(E->getSubExpr()));
  }
  void VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *E) {
    AddAnnotation(E, AnnotationOf(E->GetTemporaryExpr()));
  }

  // Visit all kinds of call expressions the same way.
  void visitCall(CallExpr *E) {
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

      AddAnnotation(E, AnnotationOf(D->getReturnType()));
    } else {
      // We couldn't determine which function is being called. Unsoundly, we
      // check nothing and return the null type. FIXME?
      DEBUG(llvm::errs() << "UNSOUND: indirect call\n");
    }
  }
  void VisitCallExpr(CallExpr *E) {
    visitCall(E);
  }
  void VisitCXXMemberCallExpr(CallExpr *E) {
    visitCall(E);
  }
  void VisitCXXOperatorCallExpr(CallExpr *E) {
    visitCall(E);
  }

  void VisitReturnStmt(ReturnStmt *S) {
    Expr *E = S->getRetValue();
    if (E) {
      assert(CurFunc && "return outside of function?");
      AssertCompatible(S, CurFunc->getReturnType(), E->getType());
    }
  }
};

// Hack to go bottom-up (postorder) on statements.
template<typename AnnotatorClass>
class TAVisitor : public RecursiveASTVisitor< TAVisitor<AnnotatorClass> > {
public:
  AnnotatorClass *Annotator;
  bool SkipHeaders;

  bool TraverseStmt(Stmt *S) {
    // Super traversal: visit children.
    RecursiveASTVisitor<TAVisitor>::TraverseStmt(S);

    // Now give type to parent.
    if (S) {
      Annotator->Visit(S);
    }

    return true;
  }

  bool TraverseDecl(Decl *D) {
    // Skip traversal of declarations in system headers.
    if (SkipHeaders) {
      auto Loc = D->getLocation();
      if (Loc.isValid()) {
        auto &Ctx = Annotator->CI.getASTContext();
        if (Ctx.getSourceManager().isInSystemHeader(Loc)) {
          // Do not traverse any children.
          return true;
        }
      }
    }

    // Tell the annotator which function it's inside.
    auto *Func = dyn_cast<FunctionDecl>(D);
    if (Func)
      Annotator->CurFunc = Func;
    bool r = RecursiveASTVisitor< TAVisitor<AnnotatorClass> >::TraverseDecl(D);
    if (Func)
      Annotator->CurFunc = NULL;
    return r;
  }

  // Disable "data recursion", which skips calls to Traverse*.
  bool shouldUseDataRecursionFor(Stmt *S) const { return false; }
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
    Visitor.SkipHeaders = true;  // TODO configurable?

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

  virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
    for (auto it : DG) {
      Visitor.TraverseDecl(it);
    }
    return true;
  }
};

}

#undef DEBUG_TYPE

#endif
