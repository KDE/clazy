/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "implicit-casts.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Linkage.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

ImplicitCasts::ImplicitCasts(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    m_filesToIgnore = {"qobject_impl.h", "qdebug.h", "hb-", "qdbusintegrator.cpp", "harfbuzz-", "qunicodetools.cpp"};
}

static bool isInterestingFunction(FunctionDecl *func)
{
    if (!func) {
        return false;
    }

    // The interesting function calls for the pointertoBool check are those having bool and also pointer arguments,
    // which might get mixed

    bool hasBoolArgument = false;
    bool hasPointerArgument = false;

    for (auto *param : Utils::functionParameters(func)) {
        const Type *t = param->getType().getTypePtrOrNull();
        hasBoolArgument |= (t && t->isBooleanType());
        hasPointerArgument |= (t && t->isPointerType());

        if (hasBoolArgument && hasPointerArgument) {
            return true;
        }
    }

    return false;
}

// Checks for pointer->bool implicit casts
template<typename T>
static bool iterateCallExpr(T *callExpr, CheckBase *check)
{
    if (!callExpr) {
        return false;
    }

    bool result = false;

    int i = 0;
    for (auto arg : callExpr->arguments()) {
        ++i;
        auto implicitCast = dyn_cast<ImplicitCastExpr>(arg);
        if (!implicitCast || implicitCast->getCastKind() != clang::CK_PointerToBoolean) {
            continue;
        }

        check->emitWarning(implicitCast->getBeginLoc(), "Implicit pointer to bool cast (argument " + std::to_string(i) + ')');
        result = true;
    }

    return result;
}

// Checks for bool->int implicit casts
template<typename T>
static bool iterateCallExpr2(T *callExpr, CheckBase *check, ParentMap *parentMap)
{
    if (!callExpr) {
        return false;
    }

    bool result = false;

    int i = 0;
    for (auto arg : callExpr->arguments()) {
        ++i;
        auto implicitCast = dyn_cast<ImplicitCastExpr>(arg);
        if (!implicitCast || implicitCast->getCastKind() != clang::CK_IntegralCast) {
            continue;
        }

        if (implicitCast->getType().getTypePtrOrNull()->isBooleanType()) {
            continue;
        }

        Expr *expr = implicitCast->getSubExpr();
        QualType qt = expr->getType();

        if (!qt.getTypePtrOrNull()->isBooleanType()) { // Filter out some bool to const bool
            continue;
        }

        if (clazy::getFirstChildOfType<CXXFunctionalCastExpr>(implicitCast)) {
            continue;
        }

        if (clazy::getFirstChildOfType<CStyleCastExpr>(implicitCast)) {
            continue;
        }

        if (Utils::isInsideOperatorCall(parentMap, implicitCast, {"QTextStream", "QAtomicInt", "QBasicAtomicInt"})) {
            continue;
        }

        if (Utils::insideCTORCall(parentMap, implicitCast, {"QAtomicInt", "QBasicAtomicInt"})) {
            continue;
        }

        check->emitWarning(implicitCast->getBeginLoc(), "Implicit bool to int cast (argument " + std::to_string(i) + ')');
        result = true;
    }

    return result;
}

void ImplicitCasts::VisitStmt(clang::Stmt *stmt)
{
    // Lets check only in function calls. Otherwise there are too many false positives, it's common
    // to implicit cast to bool when checking pointers for validity, like if (ptr)

    auto *callExpr = dyn_cast<CallExpr>(stmt);
    CXXConstructExpr *ctorExpr = nullptr;
    if (!callExpr) {
        ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
        if (!ctorExpr) {
            return;
        }
    }

    if (isa<CXXOperatorCallExpr>(stmt)) {
        return;
    }

    if (isMacroToIgnore(stmt->getBeginLoc())) {
        return;
    }

    if (shouldIgnoreFile(stmt->getBeginLoc())) {
        return;
    }

    FunctionDecl *func = callExpr ? callExpr->getDirectCallee() : ctorExpr->getConstructor();

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
    if (!func || !isOptionSet("bool-to-int")) {
        return false;
    }

    if (func->getLanguageLinkage() != CXXLanguageLinkage || func->isVariadic()) {
        return false; // Disabled for now, too many false-positives when interacting with C code
    }

    static const std::vector<std::string> functions = {"QString::arg"};
    return !clazy::contains(functions, func->getQualifiedNameAsString());
}

bool ImplicitCasts::isMacroToIgnore(SourceLocation loc) const
{
    static const std::vector<StringRef> macros = {"QVERIFY", "Q_UNLIKELY", "Q_LIKELY"};
    if (!loc.isMacroID()) {
        return false;
    }
    StringRef macro = Lexer::getImmediateMacroName(loc, sm(), lo());
    return clazy::contains(macros, macro);
}
