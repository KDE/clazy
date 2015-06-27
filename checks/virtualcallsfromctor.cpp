#include "virtualcallsfromctor.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>

using namespace std;
using namespace clang;

VirtualCallsFromCTOR::VirtualCallsFromCTOR(clang::CompilerInstance &ci)
    : CheckBase(ci)
{

}

void VirtualCallsFromCTOR::VisitStmt(clang::Stmt *stm)
{

}

void VirtualCallsFromCTOR::VisitDecl(Decl *decl)
{
    CXXConstructorDecl *ctorDecl = dyn_cast<CXXConstructorDecl>(decl);
    CXXDestructorDecl *dtorDecl = dyn_cast<CXXDestructorDecl>(decl);
    if (ctorDecl == nullptr && dtorDecl == nullptr)
        return;

    Stmt *ctorOrDtorBody = ctorDecl ? ctorDecl->getBody() : dtorDecl->getBody();
    if (ctorOrDtorBody == nullptr)
        return;

    CXXRecordDecl *classDecl = ctorDecl ? ctorDecl->getParent() : dtorDecl->getParent();

    std::vector<Stmt*> processedStmts;
    if (containsVirtualCall(classDecl, ctorOrDtorBody, processedStmts)) {
        if (ctorDecl != nullptr)
            emitWarning(ctorOrDtorBody->getLocStart(), "Calling virtual function in CTOR [-Wmore-warnings-virtual-call-ctor]");
        else
            emitWarning(ctorOrDtorBody->getLocStart(), "Calling virtual function in DTOR [-Wmore-warnings-virtual-call-dtor]");
    }
}

std::string VirtualCallsFromCTOR::name() const
{
    return "virtual-call-ctor";
}

bool VirtualCallsFromCTOR::containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<Stmt*> &processedStmts) const
{
    if (stmt == nullptr)
        return false;

    // already processed ? we don't want recurring calls
    if (std::find(processedStmts.cbegin(), processedStmts.cend(), stmt) != processedStmts.cend())
        return false;

    processedStmts.push_back(stmt);

    std::vector<CXXMemberCallExpr*> memberCalls;
    Utils::getChilds2<CXXMemberCallExpr>(stmt, memberCalls);

    for (CXXMemberCallExpr *callExpr : memberCalls) {
        CXXMethodDecl *memberDecl = callExpr->getMethodDecl();
        if (memberDecl == nullptr || dyn_cast<CXXThisExpr>(callExpr->getImplicitObjectArgument()) == nullptr)
            continue;

        if (memberDecl->getParent() == classDecl) {
            if (memberDecl->isPure()) {
                emitWarning(callExpr->getLocStart(), "Called here [-Wmore-warnings-virtual-call-ctor]");
                return true;
            } else {
                if (containsVirtualCall(classDecl, memberDecl->getBody(), processedStmts))
                    return true;
            }
        }
    }

    return false;
}
