/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "checkmanager.h"
#include "detachingtemporaries.h"
#include "Utils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingTemporaries::DetachingTemporaries(const std::string &name)
    : CheckBase(name)
{
    m_methodsByType["QList"] = {"first", "last", "begin", "end", "front", "back", "takeAt", "takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_methodsByType["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "fill", "insert" };
    m_methodsByType["QMap"] = {"begin", "end", "erase", "first", "find", "insert", "insertMulti", "last", "lowerBound", "remove", "take", "upperBound", "unite" };
    m_methodsByType["QHash"] = {"begin", "end", "erase", "find", "insert", "insertMulti", "remove", "take", "unite" };
    m_methodsByType["QLinkedList"] = {"first", "last", "begin", "end", "front", "back", "takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_methodsByType["QSet"] = {"begin", "end", "erase", "find", "insert", "intersect", "unite", "subtract"};
    m_methodsByType["QStack"] = {"push", "swap", "top"};
    m_methodsByType["QQueue"] = {"head", "enqueue", "swap"};
    m_methodsByType["QMultiMap"] = m_methodsByType["QMap"];
    m_methodsByType["QMultiHash"] = m_methodsByType["QHash"];
    m_methodsByType["QString"] = {"begin", "end", "data"};
    m_methodsByType["QByteArray"] = {"data"};
    m_methodsByType["QImage"] = {"bits", "scanLine"};
}

bool isAllowedChainedClass(const std::string &className)
{
    static const vector<string> allowed = {"QString", "QByteArray", "QVariant"};
    return find(allowed.cbegin(), allowed.cend(), className) != allowed.cend();
}

bool isAllowedChainedMethod(const std::string &methodName)
{
    static const vector<string> allowed = {"QMap::keys", "QMap::values", "QHash::keys", "QMap::values",
                                           "QApplication::topLevelWidgets", "QAbstractItemView::selectedIndexes",
                                           "QListWidget::selectedItems", "QFile::encodeName", "QFile::decodeName",
                                           "QItemSelectionModel::selectedRows", "QTreeWidget::selectedItems",
                                           "QTableWidget::selectedItems", "QNetworkReply::rawHeaderList", "Mailbox::address"};
    return find(allowed.cbegin(), allowed.cend(), methodName) != allowed.cend();
}

void DetachingTemporaries::VisitStmt(clang::Stmt *stm)
{
    CXXMemberCallExpr *memberExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (!memberExpr)
        return;

    CXXRecordDecl *classDecl = memberExpr->getRecordDecl();
    CXXMethodDecl *methodDecl = memberExpr->getMethodDecl();
    if (!classDecl || !methodDecl)
        return;

    // Check if it's one of the implicit shared classes
    const std::string className = classDecl->getNameAsString();
    auto it = m_methodsByType.find(className);
    if (it == m_methodsByType.end())
        return;

    // Check if it's one of the detaching methods
    const std::string functionName = methodDecl->getNameAsString();
    const auto &allowedFunctions = it->second;
    if (std::find(allowedFunctions.cbegin(), allowedFunctions.cend(), functionName) == allowedFunctions.cend())
        return;

    Expr *expr = memberExpr->getImplicitObjectArgument();

    if (!expr || (!expr->isRValue())) // This check is about detaching temporaries, so check for r value
        return;

    {
        // *really* check for rvalue
        ImplicitCastExpr *impl = dyn_cast<ImplicitCastExpr>(expr);
        if (impl) {
            if (impl->getCastKind() == CK_LValueToRValue)
                return;
            auto childs = Utils::childs(impl);
            if (!childs.empty() && isa<ImplicitCastExpr>(childs[0]) && dyn_cast<ImplicitCastExpr>(childs[0])->getCastKind() == CK_LValueToRValue)
                return;
        }
    }

    QualType qt = expr->getType();
    const Type *exprType = qt.getTypePtrOrNull();
    if (exprType->isPointerType()) {
        QualType qt = exprType->getPointeeType();
        if (qt.isConstQualified())
            return;
    } else if (qt.isConstQualified()) {
        return; // const doesn't detach
    }

    if (isa<CXXBindTemporaryExpr>(expr)) {
        Expr *subExpr = dyn_cast<CXXBindTemporaryExpr>(expr)->getSubExpr();
        if (subExpr) {
            CXXMemberCallExpr *chainedMemberCall = dyn_cast<CXXMemberCallExpr>(subExpr);
            if (chainedMemberCall) {
                auto record = chainedMemberCall->getRecordDecl();
                if ((record && isAllowedChainedClass(record->getNameAsString())) || isAllowedChainedMethod(StringUtils::qualifiedMethodName(chainedMemberCall->getMethodDecl())))
                    return;
            } else {
                // This is a static member call, such as QFile::encodeName()
                CallExpr *callExpr = dyn_cast<CallExpr>(subExpr);
                if (callExpr) {
                    FunctionDecl *fDecl = callExpr->getDirectCallee();
                    if (fDecl) {
                        auto name = isa<CXXMethodDecl>(fDecl) ? StringUtils::qualifiedMethodName(dyn_cast<CXXMethodDecl>(fDecl)) : fDecl->getNameAsString();
                        if (isAllowedChainedMethod(name))
                            return;
                    }
                }
            }
        }
    }

    // Check if this is a QGlobalStatic
    if (auto operatorExpr = dyn_cast<CXXOperatorCallExpr>(expr)) {
        auto method = dyn_cast_or_null<CXXMethodDecl>(operatorExpr->getDirectCallee());
        if (method && method->getParent()->getNameAsString() == "QGlobalStatic") {
            return;
        }
    }

    CXXConstructExpr *possibleCtorCall = dyn_cast_or_null<CXXConstructExpr>(Utils::getFirstChildAtDepth(expr, 2));
    if (possibleCtorCall != nullptr)
        return;

    // llvm::errs() << "Expression: " << expr->getStmtClassName() << "\n";

    std::string error = std::string("Don't call ") + StringUtils::qualifiedMethodName(methodDecl) + std::string("() on temporary");
    emitWarning(stm->getLocStart(), error.c_str());
}

REGISTER_CHECK("detaching-temporary", DetachingTemporaries)
