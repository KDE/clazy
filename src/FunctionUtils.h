/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FUNCTION_UTILS_H
#define FUNCTION_UTILS_H

// Contains utility functions regarding functions and methods

#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>

namespace clazy
{
inline clang::ValueDecl *valueDeclForCallArgument(clang::CallExpr *call, unsigned int argIndex)
{
    if (!call || call->getNumArgs() <= argIndex) {
        return nullptr;
    }

    clang::Expr *firstArg = call->getArg(argIndex);
    auto *declRef =
        llvm::isa<clang::DeclRefExpr>(firstArg) ? llvm::cast<clang::DeclRefExpr>(firstArg) : clazy::getFirstChildOfType2<clang::DeclRefExpr>(firstArg);
    if (!declRef) {
        return nullptr;
    }

    return declRef->getDecl();
}

inline bool parametersMatch(const clang::FunctionDecl *f1, const clang::FunctionDecl *f2)
{
    if (!f1 || !f2) {
        return false;
    }

    auto params1 = f1->parameters();
    auto params2 = f2->parameters();

    if (params1.size() != params2.size()) {
        return false;
    }

    for (int i = 0, e = params1.size(); i < e; ++i) {
        clang::ParmVarDecl *p1 = params1[i];
        clang::ParmVarDecl *p2 = params2[i];

        if (p1->getType() != p2->getType()) {
            return false;
        }
    }

    return true;
}

/**
 * Returns true if a class contains a method with a specific signature.
 * (method->getParent() doesn't need to equal record)
 */
inline bool classImplementsMethod(const clang::CXXRecordDecl *record, const clang::CXXMethodDecl *method)
{
    if (!method->getDeclName().isIdentifier()) {
        return false;
    }

    llvm::StringRef methodName = clazy::name(method);
    for (auto *m : record->methods()) {
#if LLVM_VERSION_MAJOR >= 18
        if (!m->isPureVirtual() && clazy::name(m) == methodName && parametersMatch(m, method)) {
#else
        if (!m->isPure() && clazy::name(m) == methodName && parametersMatch(m, method)) {
#endif
            return true;
        }
    }

    return false;
}

}
#endif
