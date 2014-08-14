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

  llvm::StringRef AnnotationOf(const Type *T) const {
    // TODO step through desugaring (typedefs, etc.)
    if (auto *AT = llvm::dyn_cast<AnnotatedType>(T)) {
      return AT->getAnnotation();
    } else {
      return StringRef();
    }
  }

  llvm::StringRef AnnotationOf(QualType QT) const {
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

  llvm::StringRef Visit(Stmt *S) {
    if (!S) {
      return StringRef();
    }
    StringRef ty = StmtVisitor<ImplClass, llvm::StringRef>::Visit(S);
    if (ty.size()) {
      llvm::errs() << "FIXME do something with the new type\n";
    }
    return ty;
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
    StringRef ty = Annotator->Visit(S);
    DEBUG(if (S && isa<Expr>(S)) S->dump());
    DEBUG(llvm::errs() << "result type: " << ty << "\n");

    return true;
  }

  // Tell the typer which function it's inside.
  bool TraverseFunctionDecl(FunctionDecl *D) {
    Annotator->CurFunc = D;
    RecursiveASTVisitor<TAVisitor>::TraverseFunctionDecl(D);
    Annotator->CurFunc = NULL;
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
