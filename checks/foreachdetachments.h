#ifndef FOREACH_DETACHMENTS_H
#define FOREACH_DETACHMENTS_H

#include "checkbase.h"

namespace clang {
class ForStmt;
class ValueDecl;
class Stmt;
}

/**
 * Finds places where you're detaching the foreach container.
 */
class ForeachDetachments : public CheckBase
{
public:
    explicit ForeachDetachments(clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stmt) override;
    std::string name() const override;
private:
    bool containsDetachments(clang::Stmt *stmt, clang::ValueDecl *containerValueDecl);
    clang::ForStmt *m_lastForStmt = nullptr;
};

#endif
