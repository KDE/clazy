#ifndef FOREACH_DETACHMENTS_H
#define FOREACH_DETACHMENTS_H

#include "checkbase.h"

namespace clang {
class ForStmt;
class ValueDecl;
class Stmt;
}

/**
 * Finds places where you're detaching the foreach container and finds places where big or
 * non-trivial types are passed by value instead of const-ref.
 */
class Foreacher : public CheckBase
{
public:
    explicit Foreacher(clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stmt) override;
    std::string name() const override;
private:
    void checkBigTypeMissingRef();
    bool containsDetachments(clang::Stmt *stmt, clang::ValueDecl *containerValueDecl);
    clang::ForStmt *m_lastForStmt = nullptr;
};

#endif
