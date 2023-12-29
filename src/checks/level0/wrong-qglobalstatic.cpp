/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "wrong-qglobalstatic.h"
#include "MacroUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "TemplateUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <memory>
#include <vector>

class ClazyContext;

using namespace clang;

WrongQGlobalStatic::WrongQGlobalStatic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void WrongQGlobalStatic::VisitStmt(clang::Stmt *stmt)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr) {
        return;
    }

    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!ctorDecl || clazy::name(ctorDecl) != "QGlobalStatic") {
        return;
    }

    SourceLocation loc = clazy::getLocStart(stmt);
    if (clazy::isInMacro(&m_astContext, loc, "Q_GLOBAL_STATIC_WITH_ARGS")) {
        return;
    }

    CXXRecordDecl *record = ctorDecl->getParent();
    std::vector<QualType> typeList = clazy::getTemplateArgumentsTypes(record);
    const Type *t = typeList.empty() ? nullptr : typeList[0].getTypePtrOrNull();
    if (!t) {
        return;
    }

    CXXRecordDecl *usersClass = t->getAsCXXRecordDecl();
    if (usersClass) {
        if (usersClass->hasTrivialDefaultConstructor() && usersClass->hasTrivialDefaultConstructor()) {
            std::string error = std::string("Don't use Q_GLOBAL_STATIC with trivial type (") + usersClass->getNameAsString() + ')';
            emitWarning(loc, error.c_str());
        }
    } else {
        // Not a class, why use Q_GLOBAL_STATIC ?
        std::string error = std::string("Don't use Q_GLOBAL_STATIC with non-class type (") + typeList[0].getAsString() + ')';
        emitWarning(loc, error.c_str());
    }
}
