/*
    SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Waqar Ahmed <waqar.ahmed@kdab.com>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "use-static-qregularexpression.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;

UseStaticQRegularExpression::UseStaticQRegularExpression(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

static MaterializeTemporaryExpr *isArgTemporaryObj(Expr *arg0)
{
    return dyn_cast_or_null<MaterializeTemporaryExpr>(arg0);
}

static VarDecl *getVarDecl(Expr *arg)
{
    auto *declRefExpr = dyn_cast<DeclRefExpr>(arg);
    declRefExpr = declRefExpr ? declRefExpr : clazy::getFirstChildOfType<DeclRefExpr>(arg);
    if (!declRefExpr) {
        return nullptr;
    }
    return dyn_cast_or_null<VarDecl>(declRefExpr->getDecl());
}

static Expr *getVarInitExpr(VarDecl *VDef)
{
    return VDef->getDefinition() ? VDef->getDefinition()->getInit() : nullptr;
}

static bool isQStringModifiedAfterCreation(clang::Stmt *expr, LangOptions lo)
{
    // This QString is the result of a .arg/.mid or similar call, check all args if they are a literal
    if (auto *methodCall = clazy::getFirstChildOfType<CXXMemberCallExpr>(expr)) {
        if (auto *callee = methodCall->getMethodDecl()) {
            if (callee->getReturnType().getAsString(lo) == "QString") {
                return true;
            }
        }
    }
    return false;
}

static bool isQStringFromStringLiteral(Expr *qstring, LangOptions lo)
{
    if (isArgTemporaryObj(qstring)) {
        // Is it compile time known QString i.e., not from a function call
        auto *qstringCtor = clazy::getFirstChildOfType<CXXConstructExpr>(qstring);
        if (!qstringCtor) {
            return false;
        }

        return clazy::getFirstChildOfType<StringLiteral>(qstringCtor);
    }

    if (auto *VD = getVarDecl(qstring)) {
        auto *stringLit = clazy::getFirstChildOfType<StringLiteral>(getVarInitExpr(VD));
        if (stringLit) {
            // If we have a string literal somewhere in there, but modify it using QString::arg or friends, we don't have a literal for th regex
            if (auto *constructExpr = clazy::getFirstChildOfType<CXXConstructExpr>(VD->getInit())) {
                return !isQStringModifiedAfterCreation(constructExpr, lo);
            } else {
                return true;
            }
        }
    }
    return false;
}

static bool isTemporaryQRegexObj(Expr *qregexVar, const LangOptions &lo)
{
    // Get the QRegularExpression ctor
    auto *ctor = clazy::getFirstChildOfType<CXXConstructExpr>(qregexVar);
    if (!ctor || ctor->getNumArgs() == 0) {
        return false;
    }

    // Check if its first arg is "QString"
    auto *qstrArg = ctor->getArg(0);
    if (!qstrArg || clazy::typeName(qstrArg->getType(), lo, true) != "QString") {
        return false;
    }

    return isQStringFromStringLiteral(qstrArg, lo) && !isQStringModifiedAfterCreation(qstrArg, lo);
}

static bool isQRegexpFromStringLiteral(VarDecl *qregexVarDecl, LangOptions lo)
{
    Expr *initExpr = getVarInitExpr(qregexVarDecl);
    if (!initExpr) {
        return false;
    }

    auto *ctorCall = dyn_cast<CXXConstructExpr>(initExpr);
    if (!ctorCall) {
        ctorCall = clazy::getFirstChildOfType<CXXConstructExpr>(initExpr);
        if (!ctorCall) {
            return false;
        }
    }

    if (ctorCall->getNumArgs() < 2) {
        return false;
    }

    auto *qstringArg = ctorCall->getArg(0);
    if (!qstringArg) {
        return false;
    }

    // For C++17, we have to put in more effort to resolving the initialization
    if (auto *expr = clazy::getFirstChildOfType<DeclRefExpr>(qstringArg)) {
        if (auto *exprClean = dyn_cast<VarDecl>(expr->getDecl())) {
            if (isQStringModifiedAfterCreation(exprClean->getInit(), lo)) {
                return false;
            }
        }
    }
    return isQStringFromStringLiteral(qstringArg, lo) && !isQStringModifiedAfterCreation(qstringArg, lo);
}

static bool isArgNonStaticLocalVar(Expr *qregexp, LangOptions lo)
{
    auto *varDecl = getVarDecl(qregexp);
    if (!varDecl) {
        return false;
    }

    if (!isQRegexpFromStringLiteral(varDecl, lo)) {
        return false;
    }

    return varDecl->isLocalVarDecl() && !varDecl->isStaticLocal();
}

static bool isOfAcceptableType(CXXMethodDecl *methodDecl)
{
    const auto type = clazy::classNameFor(methodDecl);
    return type == "QString" || type == "QStringList" || type == "QRegularExpression" || type == "QListSpecialMethods" /* for QStringList in Qt6 */;
}

static bool firstArgIsQRegularExpression(CXXMethodDecl *methodDecl, const LangOptions &lo)
{
    return clazy::simpleArgTypeName(methodDecl, 0, lo) == "QRegularExpression";
}

void UseStaticQRegularExpression::VisitStmt(clang::Stmt *stmt)
{
    if (!stmt) {
        return;
    }

    auto *method = dyn_cast_or_null<CXXMemberCallExpr>(stmt);
    if (!method) {
        return;
    }

    if (method->getNumArgs() == 0) {
        return;
    }

    auto *methodDecl = method->getMethodDecl();
    if (!methodDecl || !methodDecl->getDeclName().isIdentifier()) {
        return;
    }

    if (!isOfAcceptableType(methodDecl)) {
        return;
    }

    // QRegularExpression.match()
    const auto methodName = methodDecl->getName();
    if (methodName == "match" || methodName == "globalMatch") {
        auto *obj = method->getImplicitObjectArgument()->IgnoreImpCasts();
        if (!obj) {
            return;
        }

        if (obj->isLValue()) {
            if (isArgNonStaticLocalVar(obj, lo())) {
                emitWarning(obj->getBeginLoc(), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
                return;
            }
        } else if (obj->isXValue()) {
            // is it a temporary?
            auto *temp = dyn_cast<MaterializeTemporaryExpr>(obj);
            if (!temp) {
                return;
            }
            if (isTemporaryQRegexObj(temp, lo())) {
                emitWarning(temp->getBeginLoc(), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
            }
        }
        return;
    }

    if (!firstArgIsQRegularExpression(methodDecl, lo())) {
        return;
    }

    Expr *qregexArg = method->getArg(0);
    if (!qregexArg) {
        return;
    }

    // Its a QString*().method(QRegularExpression(arg)) ?
    if (auto *temp = isArgTemporaryObj(qregexArg)) {
        if (isTemporaryQRegexObj(temp, lo())) {
            emitWarning(qregexArg->getBeginLoc(), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
        }
    }

    // Its a local QRegularExpression variable?
    if (isArgNonStaticLocalVar(qregexArg, lo())) {
        emitWarning(qregexArg->getBeginLoc(), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
    }
}
