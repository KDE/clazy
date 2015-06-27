#include "checkbase.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMap.h>

#include <vector>

using namespace clang;

CheckBase::CheckBase(CompilerInstance &ci)
    : m_ci(ci)
{
    ASTContext &context = m_ci.getASTContext();
    m_tu = context.getTranslationUnitDecl();
    //m_parentMap = new ParentMap(m_tu->getBody());
    m_lastMethodDecl = nullptr;
}

CheckBase::~CheckBase()
{
}

void CheckBase::VisitStatement(Stmt *stm)
{
    VisitStmt(stm);
}

void CheckBase::VisitDeclaration(Decl *decl)
{
    auto mdecl = dyn_cast<CXXMethodDecl>(decl);
    if (mdecl)
        m_lastMethodDecl = mdecl;

    VisitDecl(decl);
}

void CheckBase::VisitStmt(Stmt *)
{
}

void CheckBase::VisitDecl(Decl *)
{
}

bool CheckBase::shouldIgnoreFile(const std::string &filename) const
{
/*    std::vector<std::string> files { "x86_64-unknown-linux-gnu",
                                     "qsharedpointer_impl.h",
                                     "bearer",
                                     "boost",
                                     "CMakeTmp",
                                     "ksharedptr.h",
                                     "qlist.h",
                                     "moc_", ".moc", "tst_", "qrc",
                                     "kresources" };
*/

    const std::vector<std::string> files = filesToIgnore();
    for (auto &file : files) {
        bool contains = filename.find(file) != std::string::npos;
        if (contains)
            return true;
    }

    return false;
}

std::vector<std::string> CheckBase::filesToIgnore() const
{
    return {};
}

void CheckBase::emitWarning(clang::SourceLocation loc, const char *error) const
{
    FullSourceLoc full(loc, m_ci.getSourceManager());
    unsigned id = m_ci.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, error);
    DiagnosticBuilder B = m_ci.getDiagnostics().Report(full, id);
}
