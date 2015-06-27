#ifndef VIRTUALCALLSFROMCTOR_H
#define VIRTUALCALLSFROMCTOR_H

#include "checkbase.h"

#include <vector>

namespace clang {
class CXXRecordDecl;
class Stmt;
class SourceLocation;
}

/**
 * Finds places where you're calling pure virtual functions inside a CTOR or DTOR.
 * Compilers warn about this if there isn't any indirection, this plugin will catch cases like calling
 * a non-pure virtual that calls a pure virtual.
 *
 * This plugin only checks for pure virtuals, ignoring non-pure, which in theory you shouldn't call,
 * but seems common practice.
 */
class VirtualCallsFromCTOR : public CheckBase
{
public:
    explicit VirtualCallsFromCTOR(clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stm) override;
    void VisitDecl(clang::Decl *decl) override;
    std::string name() const override;

private:
    bool containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<clang::Stmt*> &processedStmts) const;
};


#endif
