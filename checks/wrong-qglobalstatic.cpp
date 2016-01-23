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
#include "Utils.h"
#include "TemplateUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


WrongQGlobalStatic::WrongQGlobalStatic(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void WrongQGlobalStatic::VisitStmt(clang::Stmt *stmt)
{
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr)
        return;

    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!ctorDecl || ctorDecl->getNameAsString() != "QGlobalStatic")
        return;

    SourceLocation loc = stmt->getLocStart();
    if (Utils::isInMacro(m_ci, loc, "Q_GLOBAL_STATIC_WITH_ARGS"))
        return;

    CXXRecordDecl *record = ctorDecl->getParent();
    vector<QualType> typeList = TemplateUtils::getTemplateArgumentsTypes(record);
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

REGISTER_CHECK_WITH_FLAGS("wrong-qglobalstatic", WrongQGlobalStatic, CheckLevel0)
