/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "implicitcasts.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


ImplicitCasts::ImplicitCasts(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    m_filesToIgnore = { "qobject_impl.h", "qdebug.h", "hb-", "qdbusintegrator.cpp",
                        "harfbuzz-", "qunicodetools.cpp" };
}

static bool isInterestingFunction(FunctionDecl *func)
{
    if (!func)
        return false;

    // The interesting function calls for the pointertoBool check are those having bool and also pointer arguments,
    // which might get mixed

    bool hasBoolArgument = false;
    bool hasPointerArgument = false;

    for (auto param : Utils::functionParameters(func)) {
        const Type *t = param->getType().getTypePtrOrNull();
        hasBoolArgument |= (t && t->isBooleanType());
        hasPointerArgument |= (t && t->isPointerType());

        if (hasBoolArgument && hasPointerArgument)
            return true;
    }

    return false;
}

// Checks for pointer->bool implicit casts
template <typename T>
static bool iterateCallExpr(T* callExpr, CheckBase *check)
{
    if (!callExpr)
        return false;

    bool result = false;

    int i = 0;
    for (auto arg : callExpr->arguments()) {
        ++i;
        auto implicitCast = dyn_cast<ImplicitCastExpr>(arg);
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
    for (auto arg : callExpr->arguments()) {
        ++i;
        auto implicitCast = dyn_cast<ImplicitCastExpr>(arg);
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
    // Lets check only in function calls. Otherwise there are too many false positives, it's common
    // to implicit cast to bool when checking pointers for validity, like if (ptr)

    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    auto ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!callExpr && !ctorExpr)
        return;

    if (isa<CXXOperatorCallExpr>(stmt))
        return;

    if (isMacroToIgnore(stmt->getLocStart()))
        return;

    if (shouldIgnoreFile(stmt->getLocStart()))
        return;

    FunctionDecl *func = callExpr ? callExpr->getDirectCallee()
                                  : ctorExpr->getConstructor();


    if (isInterestingFunction(func)) {
        // Check pointer->bool implicit casts
        iterateCallExpr<CallExpr>(callExpr, this);
        iterateCallExpr<CXXConstructExpr>(ctorExpr, this);
    } else if (isBoolToInt(func)) {
        // Check bool->int implicit casts
        iterateCallExpr2<CallExpr>(callExpr, this, m_context->parentMap);
        iterateCallExpr2<CXXConstructExpr>(ctorExpr, this, m_context->parentMap);
    }
}

bool ImplicitCasts::isBoolToInt(FunctionDecl *func) const
{
    if (!func || !isOptionSet("bool-to-int"))
        return false;

    if (func->getLanguageLinkage() != CXXLanguageLinkage || func->isVariadic())
        return false; // Disabled for now, too many false-positives when interacting with C code

    static const vector<string> functions = {"QString::arg"};
    return !clazy_std::contains(functions, func->getQualifiedNameAsString());
}

bool ImplicitCasts::isMacroToIgnore(SourceLocation loc) const
{
    static const vector<string> macros = {"QVERIFY",  "Q_UNLIKELY", "Q_LIKELY"};
    auto macro = Lexer::getImmediateMacroName(loc, sm(), lo());
    return clazy_std::contains(macros, macro);
}

std::vector<string> ImplicitCasts::supportedOptions() const
{
    static const vector<string> options = { "bool-to-int" };
    return options;
}

REGISTER_CHECK("implicit-casts", ImplicitCasts, CheckLevel2)
