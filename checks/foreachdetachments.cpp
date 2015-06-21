#include "foreachdetachments.h"
#include "Utils.h"

using namespace clang;
using namespace std;

const std::map<std::string, std::vector<std::string> > & detachingMethodsMap()
{
    static std::map<std::string, std::vector<std::string> > methodsMap;
    if (methodsMap.empty()) {
        methodsMap["QList"] = {"first", "last", "begin", "end", "front", "back"};
        methodsMap["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "fill"};
        methodsMap["QMap"] = {"begin", "end", "first", "find", "last", "lowerBound", "upperBound"};
        methodsMap["QHash"] = {"begin", "end", "find"};
        methodsMap["QLinkedList"] = {"first", "last", "begin", "end", "front", "back"};
        methodsMap["QSet"] = {"begin", "end", "find"};
        methodsMap["QStack"] = {"top"};
        methodsMap["QQueue"] = {"head"};
        methodsMap["QMultiMap"] = methodsMap["QMap"];
        methodsMap["QMultiHash"] = methodsMap["QHash"];
        methodsMap["QString"] = {"begin", "end", "data"};
        methodsMap["QByteArray"] = {"data"};
        methodsMap["QImage"] = {"bits", "scanLine"};
    }

    return methodsMap;
}

ForeachDetachments::ForeachDetachments(clang::CompilerInstance &ci)
    : CheckBase(ci)
{

}

void ForeachDetachments::VisitStmt(clang::Stmt *stmt)
{
    auto forStm = dyn_cast<ForStmt>(stmt);
    if (forStm != nullptr) {
        m_lastForStmt = forStm;
        return;
    }

    if (m_lastForStmt == nullptr)
        return;

    CXXConstructExpr *constructExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (constructExpr == nullptr)
        return;

    CXXConstructorDecl *constructorDecl = constructExpr->getConstructor();
    if (constructorDecl == nullptr || constructorDecl->getNameAsString() != "QForeachContainer")
        return;

    vector<DeclRefExpr*> declRefExprs;
    Utils::getChilds2<DeclRefExpr>(constructExpr, declRefExprs);
    if (declRefExprs.empty())
        return;

    // Get the container value declaration
    DeclRefExpr *declRefExpr = declRefExprs.front();
    ValueDecl *valueDecl = dyn_cast<ValueDecl>(declRefExpr->getDecl());
    if (valueDecl == nullptr)
        return;

    // const containers are fine
    if (valueDecl->getType().isConstQualified())
        return;

    // Now look inside the for statement for detachments
    if (containsDetachments(m_lastForStmt, valueDecl)) {
        emitWarning(stmt->getLocStart(), "foreach container detached [-Wmore-warnings-foreach-detachment]");
    }
}


std::string ForeachDetachments::name() const
{
    return "foreach-detachment";
}

bool ForeachDetachments::containsDetachments(Stmt *stm, clang::ValueDecl *containerValueDecl)
{
    if (stm == nullptr)
        return false;

    auto memberExpr = dyn_cast<MemberExpr>(stm);
    if (memberExpr != nullptr) {
        ValueDecl *valDecl = memberExpr->getMemberDecl();
        if (valDecl != nullptr && valDecl->isCXXClassMember()) {
            DeclContext *declContext = valDecl->getDeclContext();
            auto recordDecl = dyn_cast<CXXRecordDecl>(declContext);
            if (recordDecl != nullptr) {
                const std::string className = recordDecl->getQualifiedNameAsString();
                if (detachingMethodsMap().find(className) != detachingMethodsMap().end()) {
                    const std::string functionName = valDecl->getNameAsString();
                    const auto &allowedFunctions = detachingMethodsMap().at(className);
                    if (std::find(allowedFunctions.cbegin(), allowedFunctions.cend(), functionName) != allowedFunctions.cend()) {
                        Expr *expr = memberExpr->getBase();
                        if (expr && llvm::isa<DeclRefExpr>(expr)) {
                            if (dyn_cast<DeclRefExpr>(expr)->getDecl() == containerValueDecl) { // Finally, check if this non-const member call is on the same container we're iterating
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    auto it = stm->child_begin();
    auto end = stm->child_end();
    for (; it != end; ++it) {
        if (containsDetachments(*it, containerValueDecl))
            return true;
    }

    return false;
}
