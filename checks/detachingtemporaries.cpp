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

#include "detachingtemporaries.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingTemporaries::DetachingTemporaries(CompilerInstance &ci)
    : CheckBase(ci)
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

void DetachingTemporaries::VisitStmt(clang::Stmt *stm)
{
    auto memberExpr = dyn_cast<MemberExpr>(stm);
    if (memberExpr == nullptr)
        return;

    ValueDecl *valueDecl = memberExpr->getMemberDecl();
    if (valueDecl == nullptr || !valueDecl->isCXXClassMember())
        return;

    QualType qt = valueDecl->getType();
    const Type *type = qt.getTypePtrOrNull();
    if (type == nullptr)
        return;

    const auto functionType = dyn_cast<FunctionType>(type);
    if (functionType == nullptr)
        return;

    QualType return_qt = functionType->getReturnType();
    const Type *return_type = return_qt.getTypePtrOrNull();
    if (return_type == nullptr)
        return;

    DeclContext *declContext = valueDecl->getDeclContext();
    auto recordDecl = dyn_cast<CXXRecordDecl>(declContext);
    if (recordDecl == nullptr)
        return;

    const std::string className = recordDecl->getQualifiedNameAsString();
    if (m_methodsByType.find(className) == m_methodsByType.end())
        return;

    const std::string functionName = valueDecl->getNameAsString();
    const auto &allowedFunctions = m_methodsByType[className];
    if (std::find(allowedFunctions.cbegin(), allowedFunctions.cend(), functionName) == allowedFunctions.cend()) {
        return;
    }

    if (stm->children().begin() != stm->child_end()) {
        auto temporary = dyn_cast<CXXBindTemporaryExpr>(*stm->child_begin());
        if (temporary) {
            std::string error = std::string("Don't call ") + className + "::" + functionName + std::string("() on temporary");
            emitWarning(stm->getLocStart(), error.c_str());
        }
    }
}

std::string DetachingTemporaries::name() const
{
    return "detaching-temporary";
}
