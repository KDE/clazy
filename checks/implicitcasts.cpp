/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "implicitcasts.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


ImplicitCasts::ImplicitCasts(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{

}

std::vector<string> ImplicitCasts::filesToIgnore() const
{
    static vector<string> files = {"/gcc/", "/c++/", "functional_hash.h", "qobject_impl.h", "qdebug.h",
                                   "hb-", "qdbusintegrator.cpp", "harfbuzz-", "qunicodetools.cpp"};
    return files;
}

static bool isInterestingFunction(FunctionDecl *func)
{
    if (!func)
        return false;

    // The interesting function calls for the pointertoBool check are those having bool and also pointer arguments,
    // which might get mixed

    bool hasBoolArgument = false;
    bool hasPointerArgument = false;

    for (auto it = func->param_begin(), end = func->param_end(); it != end; ++it) {

        ParmVarDecl *param = *it;
        const Type *t = param->getType().getTypePtrOrNull();
        hasBoolArgument |= (t && t->isBooleanType());
        hasPointerArgument |= (t && t->isPointerType());

        if (hasBoolArgument && hasPointerArgument)
            return true;
    }

    return false;
}

static bool isInterestingFunction2(FunctionDecl *func)
{
    return false; // Disabled for now, too many false-positives when interacting with C code

    if (!func)
        return false;

    static const vector<string> functions = {"QString::arg"};
    return find(functions.cbegin(), functions.cend(), func->getQualifiedNameAsString()) == functions.cend();
}

// Checks for pointer->bool implicit casts
template <typename T>
static bool iterateCallExpr(T* callExpr, CheckBase *check)
{
    if (!callExpr)
        return false;

    bool result = false;

    int i = 0;
    for (auto it = callExpr->arg_begin(), end = callExpr->arg_end(); it != end; ++it) {
        ++i;
        auto implicitCast = dyn_cast<ImplicitCastExpr>(*it);
        if (!implicitCast || implicitCast->getCastKind() != clang::CK_PointerToBoolean)
            continue;

        check->emitWarning(implicitCast->getLocStart(), "Implicit pointer to bool cast (argument " + std::to_string(i) + ')');
        result = true;
    }

    return result;
}

// Checks for bool->int implicit casts
template <typename T>
static bool iterateCallExpr2(T* callExpr, CheckBase *check, ParentMap *parentMap)
{
    if (!callExpr)
        return false;

    bool result = false;

    int i = 0;
    for (auto it = callExpr->arg_begin(), end = callExpr->arg_end(); it != end; ++it) {
        ++i;
        auto implicitCast = dyn_cast<ImplicitCastExpr>(*it);
        if (!implicitCast || implicitCast->getCastKind() != clang::CK_IntegralCast)
            continue;

        if (implicitCast->getType().getTypePtrOrNull()->isBooleanType())
            continue;

        Expr *expr = implicitCast->getSubExpr();
        QualType qt = expr->getType();

        if (!qt.getTypePtrOrNull()->isBooleanType()) // Filter out some bool to const bool
            continue;

        if (HierarchyUtils::getFirstChildOfType<CXXFunctionalCastExpr>(implicitCast))
            continue;

        if (HierarchyUtils::getFirstChildOfType<CStyleCastExpr>(implicitCast))
            continue;

        if (Utils::isInsideOperatorCall(parentMap, implicitCast, {"QTextStream", "QAtomicInt", "QBasicAtomicInt"}))
            continue;

        if (Utils::insideCTORCall(parentMap, implicitCast, {"QAtomicInt", "QBasicAtomicInt"}))
            continue;

        check->emitWarning(implicitCast->getLocStart(), "Implicit bool to int cast (argument " + std::to_string(i) + ')');
        result = true;
    }

    return result;
}

void ImplicitCasts::VisitStmt(clang::Stmt *stmt)
{
    if (isMacroToIgnore(stmt->getLocStart()))
        return;

    // Lets check only in function calls. Otherwise there are too many false positives, it's common
    // to implicit cast to bool when checking pointers for validity, like if (ptr)

    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!callExpr && !ctorExpr)
        return;

    FunctionDecl *func = callExpr ? callExpr->getDirectCallee()
                                  : ctorExpr->getConstructor();


    if (isInterestingFunction(func)) {
        // Check pointer->bool implicit casts
        iterateCallExpr<CallExpr>(callExpr, this);
        iterateCallExpr<CXXConstructExpr>(ctorExpr, this);
    } else if (isInterestingFunction2(func)) {
        // Check bool->int implicit casts
        iterateCallExpr2<CallExpr>(callExpr, this, m_parentMap);
        iterateCallExpr2<CXXConstructExpr>(ctorExpr, this, m_parentMap);
    }
}

bool ImplicitCasts::isMacroToIgnore(SourceLocation loc) const
{
    static const vector<string> macros = {"QVERIFY",  "Q_UNLIKELY", "Q_LIKELY"};
    auto macro = Lexer::getImmediateMacroName(loc, m_ci.getSourceManager(), m_ci.getLangOpts());
    return find(macros.cbegin(), macros.cend(), macro) != macros.cend();
}


REGISTER_CHECK_WITH_FLAGS("implicit-casts", ImplicitCasts, CheckLevel2)
