/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "wrong-qglobalstatic.h"
#include "MacroUtils.h"
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
    if (!ctorDecl) {
        return;
    }
    if (StringRef name = clazy::name(ctorDecl); name != "QGlobalStatic" && name != "QGlobalStaticCompoundStmt") {
        return; // Only consider relevant Qt5 and Qt6 ctors
    }

    SourceLocation loc = stmt->getBeginLoc();
    if (clazy::isInMacro(&m_astContext, loc, "Q_GLOBAL_STATIC_WITH_ARGS")) {
        return;
    }

    CXXRecordDecl *record = ctorDecl->getParent();
    std::vector<QualType> typeList = clazy::getTemplateArgumentsTypes(record);
    CXXRecordDecl *usersClass = nullptr;
    std::string underlyingTypeName;
    if (typeList.empty()) {
        return;
    }
    if (clazy::classNameFor(typeList[0]) == "Holder") { // In Qt6, we need to look into the Holder for the user-defined class/type name
        auto templateTypes = clazy::getTemplateArgumentsTypes(typeList[0]->getAsCXXRecordDecl());
        if (templateTypes.empty()) {
            return;
        }
        QualType qgsType = templateTypes[0];
        if (auto *typePtr = qgsType.getTypePtrOrNull(); typePtr && typePtr->isRecordType()) {
            for (auto *decl : typePtr->getAsCXXRecordDecl()->decls()) {
                if (auto *typedefDecl = dyn_cast<TypedefDecl>(decl); typedefDecl && typedefDecl->getNameAsString() == "QGS_Type") {
                    usersClass = typedefDecl->getUnderlyingType()->getAsCXXRecordDecl();
                    underlyingTypeName = typedefDecl->getUnderlyingType().getAsString();
                    break;
                }
            }
        }
    } else if (auto *t = typeList[0].getTypePtrOrNull()) {
        usersClass = t->getAsCXXRecordDecl();
        underlyingTypeName = typeList[0].getAsString();
    }

    if (usersClass) {
        if (usersClass->hasTrivialDefaultConstructor() && usersClass->hasTrivialDestructor()) {
            emitWarning(loc, "Don't use Q_GLOBAL_STATIC with trivial type (" + usersClass->getNameAsString() + ')');
        }
    } else {
        // Not a class, why use Q_GLOBAL_STATIC ?
        emitWarning(loc, "Don't use Q_GLOBAL_STATIC with non-class type (" + underlyingTypeName + ')');
    }
}
