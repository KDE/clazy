/*
   This file is part of the clang-lazy static checker.

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

#include "qstringarg.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <vector>

using namespace clang;
using namespace std;

StringArg::StringArg(const std::string &name)
    : CheckBase(name)
{

}

static bool stringContains(const string &needle, const string &haystack)
{
    string loweredHaystack = haystack;
    std::transform(loweredHaystack.begin(), loweredHaystack.end(), loweredHaystack.begin(), ::tolower);
    return loweredHaystack.find(needle) != string::npos;
}

static string variableNameFromArg(Expr *arg)
{
    vector<DeclRefExpr*> declRefs;
    Utils::getChilds2<DeclRefExpr>(arg, declRefs);
    if (declRefs.size() == 1) {
        ValueDecl *decl = declRefs.at(0)->getDecl();
        return decl ? decl->getNameAsString() : string();
    }

    return {};
}

void StringArg::VisitStmt(clang::Stmt *stmt)
{
    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall)
        return;

    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!method || method->getNameAsString() != "arg")
        return;

    CXXRecordDecl *record = memberCall->getRecordDecl();
    if (!record || record->getNameAsString() != "QString")
        return;

    ParmVarDecl *lastParam = method->getParamDecl(method->getNumParams() - 1);
    if (lastParam && lastParam->getType().getAsString() == "class QChar") {
        // The second arg wasn't passed, so this is a safe and unambiguous use, like .arg(1)
        if (isa<CXXDefaultArgExpr>(memberCall->getArg(1)))
            return;

        ParmVarDecl *p = method->getParamDecl(2);
        if (p && p->getNameAsString() == "base") {
            // User went through the trouble specifying a base, lets allow it if it's a literal.
            vector<IntegerLiteral*> literals;
            Utils::getChilds2<IntegerLiteral>(memberCall->getArg(2), literals);
            if (!literals.empty())
                return;

            string variableName = variableNameFromArg(memberCall->getArg(2));
            if (!variableName.empty()) {
                if (stringContains("base", variableName))
                    return;
            }
        }

        p = method->getParamDecl(1);
        if (p && p->getNameAsString() == "fieldWidth") {
            // He specified a literal, so he knows what he's doing, otherwise he would have put it directly in the string
            vector<IntegerLiteral*> literals;
            Utils::getChilds2<IntegerLiteral>(memberCall->getArg(1), literals);
            if (!literals.empty())
                return;

            // the variable is named "width", user knows what he's doing
            string variableName = variableNameFromArg(memberCall->getArg(1));
            if (!variableName.empty()) {
                if (stringContains("width", variableName))
                    return;
            }
        }

        emitWarning(stmt->getLocStart(), "Using QString::arg() with fillChar overload");
    }
}

std::vector<std::string> StringArg::filesToIgnore() const
{
    return { "qstring.h" };
}


REGISTER_CHECK("qstring-arg", StringArg)
