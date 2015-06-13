#ifndef CHECK_BASE_H
#define CHECK_BASE_H

#include <clang/Frontend/CompilerInstance.h>

namespace clang {
class CXXMethodDecl;
class Stmt;
class Decl;
class ParentMap;
class TranslationUnitDecl;
}

class CheckBase
{
public:
    explicit CheckBase(clang::CompilerInstance &m_ci);
    ~CheckBase();

    void VisitStatement(clang::Stmt *stm);
    void VisitDeclaration(clang::Decl *stm);

    virtual std::string name() const = 0;

protected:
    virtual void VisitStmt(clang::Stmt *stm);
    virtual void VisitDecl(clang::Decl *decl);
    bool shouldIgnoreFile(const std::string &filename) const;
    virtual std::vector<std::string> filesToIgnore() const;
    void emitWarning(clang::SourceLocation loc, const char *error);

    clang::CompilerInstance &m_ci;
    clang::TranslationUnitDecl *m_tu;
    clang::ParentMap *m_parentMap;
    clang::CXXMethodDecl *m_lastMethodDecl;
};

#endif
