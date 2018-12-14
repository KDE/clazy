/*
    This file is part of the clazy static checker.

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

#include "wrong-qglobalstatic.h"
#include "TemplateUtils.h"
#include "MacroUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"

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
using namespace std;


WrongQGlobalStatic::WrongQGlobalStatic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void WrongQGlobalStatic::VisitStmt(clang::Stmt *stmt)
{
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr)
        return;

    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!ctorDecl || clazy::name(ctorDecl) != "QGlobalStatic")
        return;

    SourceLocation loc = clazy::getLocStart(stmt);
    if (clazy::isInMacro(&m_astContext, loc, "Q_GLOBAL_STATIC_WITH_ARGS"))
        return;

    CXXRecordDecl *record = ctorDecl->getParent();
    vector<QualType> typeList = clazy::getTemplateArgumentsTypes(record);
    const Type *t = typeList.empty() ? nullptr : typeList[0].getTypePtrOrNull();
    if (!t)
        return;

    CXXRecordDecl *usersClass = t->getAsCXXRecordDecl();
    if (usersClass) {
        if (usersClass->hasTrivialDefaultConstructor() && usersClass->hasTrivialDefaultConstructor()) {
            string error = string("Don't use Q_GLOBAL_STATIC with trivial type (") + usersClass->getNameAsString() + ')';
            emitWarning(loc, error.c_str());
        }
    } else {
        // Not a class, why use Q_GLOBAL_STATIC ?
        string error = string("Don't use Q_GLOBAL_STATIC with non-class type (") + typeList[0].getAsString()  + ')';
        emitWarning(loc, error.c_str());
    }
}
