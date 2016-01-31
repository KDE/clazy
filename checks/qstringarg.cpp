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

#include "qstringarg.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <vector>

using namespace clang;
using namespace std;

StringArg::StringArg(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{

}

static string variableNameFromArg(Expr *arg)
{
    vector<DeclRefExpr*> declRefs;
    HierarchyUtils::getChilds<DeclRefExpr>(arg, declRefs);
    if (declRefs.size() == 1) {
        ValueDecl *decl = declRefs.at(0)->getDecl();
        return decl ? decl->getNameAsString() : string();
    }

    return {};
}

static CXXMethodDecl* isArgMethod(FunctionDecl *func)
{
    if (!func)
        return nullptr;

    CXXMethodDecl *method = dyn_cast<CXXMethodDecl>(func);
    if (!method || method->getNameAsString() != "arg")
        return nullptr;

    CXXRecordDecl *record = method->getParent();
    if (!record || record->getNameAsString() != "QString")
        return nullptr;

    return method;
}

static bool isArgFuncWithOnlyQString(CallExpr *callExpr)
{
    if (!callExpr)
        return false;

    CXXMethodDecl *method = isArgMethod(callExpr->getDirectCallee());
    if (!method)
        return false;

    ParmVarDecl *secondParam = method->getParamDecl(1);
    if (classNameFor(secondParam) == "QString")
        return true;

    ParmVarDecl *firstParam = method->getParamDecl(0);
    if (classNameFor(firstParam) != "QString")
        return false;

    // This is a arg(QString, int, QChar) call, it's good if the second parameter is a default param
    return isa<CXXDefaultArgExpr>(callExpr->getArg(1));
}

bool StringArg::checkMultiArgWarningCase(const vector<clang::CallExpr *> &calls)
{
    const int size = calls.size();
    for (int i = 1; i < size; ++i) {
        auto call = calls.at(i);
        if (calls.at(i - 1)->getNumArgs() + call->getNumArgs() <= 9) {
            emitWarning(call->getLocEnd(), "Use multi-arg instead");
            return true;
        }
    }

    return false;
}

void StringArg::checkForMultiArgOpportunities(CXXMemberCallExpr *memberCall)
{
    if (!isArgFuncWithOnlyQString(memberCall))
        return;

    vector<clang::CallExpr *> callExprs = Utils::callListForChain(memberCall);
    vector<clang::CallExpr *> argCalls;
    for (auto call : callExprs) {
        if (!clazy_std::contains(m_alreadyProcessedChainedCalls, call) && isArgFuncWithOnlyQString(call)) {
            argCalls.push_back(call);
            m_alreadyProcessedChainedCalls.push_back(call);
        } else {
            if (checkMultiArgWarningCase(argCalls))
                return;
            argCalls.clear();
        }
    }

    checkMultiArgWarningCase(argCalls);
}

void StringArg::VisitStmt(clang::Stmt *stmt)
{
    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall)
        return;

    checkForMultiArgOpportunities(memberCall);

    if (!isOptionSet("fillChar-overloads"))
        return;

    CXXMethodDecl *method = isArgMethod(memberCall->getDirectCallee());
    if (!method)
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
            HierarchyUtils::getChilds<IntegerLiteral>(memberCall->getArg(2), literals);
            if (!literals.empty())
                return;

            string variableName = clazy_std::toLower(variableNameFromArg(memberCall->getArg(2)));
            if (clazy_std::contains(variableName, "base"))
                return;
        }

        p = method->getParamDecl(1);
        if (p && p->getNameAsString() == "fieldWidth") {
            // He specified a literal, so he knows what he's doing, otherwise he would have put it directly in the string
            vector<IntegerLiteral*> literals;
            HierarchyUtils::getChilds<IntegerLiteral>(memberCall->getArg(1), literals);
            if (!literals.empty())
                return;

            // the variable is named "width", user knows what he's doing
            string variableName = clazy_std::toLower(variableNameFromArg(memberCall->getArg(1)));
            if (clazy_std::contains(variableName, "width"))
                return;
        }

        emitWarning(stmt->getLocStart(), "Using QString::arg() with fillChar overload");
    }
}

std::vector<std::string> StringArg::filesToIgnore() const
{
    return { "qstring.h" };
}

std::vector<string> StringArg::supportedOptions() const
{
    static const vector<string> options = { "fillChar-overloads" };
    return options;
}


REGISTER_CHECK_WITH_FLAGS("qstring-arg", StringArg, CheckLevel0)
