/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015 Nyall Dawson <nyall.dawson@gmail.com>
    Copyright (C) 2015-2016 Sergio Martins <smartins@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#include "ClazyContext.h"
#include "temporary-iterator.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/ParentMap.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Decl.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <utility>

using namespace clang;
using namespace std;

TemporaryIterator::TemporaryIterator(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    m_methodsByType["vector"] = {"begin", "end", "cbegin", "cend" }; // TODO: More stl support
    m_methodsByType["QList"] = { "begin", "end", "constBegin", "constEnd", "cbegin", "cend" };
    m_methodsByType["QVector"] = { "begin", "end", "constBegin", "constEnd", "cbegin", "cend", "insert" };
    m_methodsByType["QMap"] = {"begin", "end", "constBegin", "constEnd", "find", "constFind", "lowerBound", "upperBound", "cbegin", "cend", "equal_range" };
    m_methodsByType["QHash"] = {"begin", "end", "constBegin", "constEnd", "cbegin", "cend", "find", "constFind", "insert", "insertMulti" };
    m_methodsByType["QLinkedList"] = {"begin", "end", "constBegin", "constEnd", "cbegin", "cend"};
    m_methodsByType["QSet"] = {"begin", "end", "constBegin", "constEnd", "find", "constFind", "cbegin", "cend"};
    m_methodsByType["QStack"] = m_methodsByType["QVector"];
    m_methodsByType["QQueue"] = m_methodsByType["QList"];
    m_methodsByType["QMultiMap"] = m_methodsByType["QMap"];
    m_methodsByType["QMultiHash"] = m_methodsByType["QHash"];
}

static bool isBlacklistedFunction(const string &name)
{
    // These are fine
    static const vector<string> list = {"QVariant::toList", "QHash::operator[]", "QMap::operator[]", "QSet::operator[]"};
    return clazy::contains(list, name);
}

void TemporaryIterator::VisitStmt(clang::Stmt *stm)
{
    auto memberExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (!memberExpr)
        return;

    CXXRecordDecl *classDecl = memberExpr->getRecordDecl();
    CXXMethodDecl *methodDecl = memberExpr->getMethodDecl();
    if (!classDecl || !methodDecl)
        return;

    // Check if it's a container
    auto it = m_methodsByType.find(clazy::name(classDecl));
    if (it == m_methodsByType.end())
        return;

    // Check if it's a method returning an iterator
    const StringRef functionName = clazy::name(methodDecl);
    const auto &allowedFunctions = it->second;
    if (!clazy::contains(allowedFunctions, functionName))
        return;


    // Catch getList().cbegin().value(), which is ok
    if (clazy::getFirstParentOfType<CXXMemberCallExpr>(m_context->parentMap, m_context->parentMap->getParent(memberExpr)))
        return;

    // Catch variant.toList().cbegin(), which is ok
    auto chainedMemberCall = clazy::getFirstChildOfType<CXXMemberCallExpr>(memberExpr);
    if (chainedMemberCall) {
        if (isBlacklistedFunction(clazy::qualifiedMethodName(chainedMemberCall->getMethodDecl())))
            return;
    }

    // catch map[foo].cbegin()
    CXXOperatorCallExpr *chainedOperatorCall = clazy::getFirstChildOfType<CXXOperatorCallExpr>(memberExpr);
    if (chainedOperatorCall) {
        FunctionDecl *func = chainedOperatorCall->getDirectCallee();
        if (func) {
            auto method = dyn_cast<CXXMethodDecl>(func);
            if (method) {
                if (isBlacklistedFunction(clazy::qualifiedMethodName(method)))
                    return;
            }
        }
    }

    // If we deref it within the expression, then we'll copy the value before the iterator becomes invalid, so it's safe
    if (Utils::isInDerefExpression(memberExpr, m_context->parentMap))
        return;

    Expr *expr = memberExpr->getImplicitObjectArgument();
    if (!expr || expr->isLValue()) // This check is about detaching temporaries, so check for r value
        return;

    const Type *containerType = expr->getType().getTypePtrOrNull();
    if (!containerType || containerType->isPointerType())
        return;

    {
        // *really* check for rvalue
        ImplicitCastExpr *impl = dyn_cast<ImplicitCastExpr>(expr);
        if (impl) {
            if (impl->getCastKind() == CK_LValueToRValue)
                return;

            Stmt *firstChild = clazy::getFirstChild(impl);
            if (firstChild && isa<ImplicitCastExpr>(firstChild) && dyn_cast<ImplicitCastExpr>(firstChild)->getCastKind() == CK_LValueToRValue)
                return;
        }
    }

    CXXConstructExpr *possibleCtorCall = dyn_cast_or_null<CXXConstructExpr>(clazy::getFirstChildAtDepth(expr, 2));
    if (possibleCtorCall)
        return;

    CXXThisExpr *possibleThisCall = dyn_cast_or_null<CXXThisExpr>(clazy::getFirstChildAtDepth(expr, 1));
    if (possibleThisCall)
        return;

    // llvm::errs() << "Expression: " << expr->getStmtClassName() << "\n";

    std::string error = std::string("Don't call ") + clazy::qualifiedMethodName(methodDecl) + std::string("() on temporary");
    emitWarning(clazy::getLocStart(stm), error.c_str());
}
