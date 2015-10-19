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

#include "qgetenv.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


QGetEnv::QGetEnv(const std::string &name)
    : CheckBase(name)
{

}



void QGetEnv::VisitStmt(clang::Stmt *stmt)
{
    // Lets check only in function calls. Otherwise there are too many false positives, it's common
    // to implicit cast to bool when checking pointers for validity, like if (ptr)

    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall)
        return;

    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!method)
        return;

    CXXRecordDecl *record = method->getParent();
    if (!record || record->getNameAsString() != "QByteArray") {
        return;
    }

    std::vector<CallExpr *> calls = Utils::callListForChain(memberCall);
    if (calls.size() != 2)
        return;

    FunctionDecl *func = calls.back()->getDirectCallee();

    if (!func || func->getNameAsString() != "qgetenv")
        return;

    string methodname = method->getNameAsString();
    string errorMsg;
    if (methodname == "isEmpty") {
        errorMsg = "qgetenv().isEmpty() allocates. Use qEnvironmentVariableIsEmpty() instead";
    } else if (methodname == "isNull") {
        errorMsg = "qgetenv().isEmpty() allocates. Use qEnvironmentVariableIsSet() instead";
    }

    if (!errorMsg.empty()) {
        emitWarning(memberCall->getLocStart(), errorMsg.c_str());
    }

}



REGISTER_CHECK("qgetenv", QGetEnv)
