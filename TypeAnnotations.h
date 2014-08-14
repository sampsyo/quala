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
class Typer : public StmtVisitor<ImplClass, llvm::StringRef> {
public:
  CompilerInstance &CI;
  ImplClass *impl;
  FunctionDecl *CurFunc;
  bool Instrument;

  Typer(CompilerInstance &_ci, bool _instrument) :
    CI(_ci),
    impl(static_cast<ImplClass*>(this)),
    CurFunc(NULL),
    Instrument(_instrument)
  {};

  llvm::StringRef TypeOf(Expr *E) {
    llvm::errs() << "FIXME look up the type\n";
    return StringRef();
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
template<typename TyperClass>
class TAVisitor : public RecursiveASTVisitor< TAVisitor<TyperClass> > {
public:
  bool TraverseStmt(Stmt *S) {
    // Super traversal: visit children.
    RecursiveASTVisitor<TAVisitor>::TraverseStmt(S);

    // Now give type to parent.
    StringRef ty = Typer->Visit(S);
    DEBUG(if (S && isa<Expr>(S)) S->dump());
    DEBUG(llvm::errs() << "result type: " << ty << "\n");

    return true;
  }

  // Tell the typer which function it's inside.
  bool TraverseFunctionDecl(FunctionDecl *D) {
    Typer->CurFunc = D;
    RecursiveASTVisitor<TAVisitor>::TraverseFunctionDecl(D);
    Typer->CurFunc = NULL;
    return true;
  }

  // Disable "data recursion", which skips calls to Traverse*.
  bool shouldUseDataRecursionFor(Stmt *S) const { return false; }

  TyperClass *Typer;
};

template<typename TyperClass>
class TAConsumer : public ASTConsumer {
public:
  CompilerInstance &CI;
  TAVisitor<TyperClass> Visitor;
  TyperClass Typer;
  bool Instrument;

  TAConsumer(CompilerInstance &_ci, bool _instrument) :
    CI(_ci),
    Typer(_ci, _instrument),
    Instrument(_instrument)
    {}

  virtual void Initialize(ASTContext &Context) {
    Visitor.Typer = &Typer;

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
