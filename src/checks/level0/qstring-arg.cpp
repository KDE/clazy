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

#include "qstring-arg.h"
#include "Utils.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"

#include <clang/AST/AST.h>
#include <vector>

using namespace clang;
using namespace std;

QStringArg::QStringArg(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    m_filesToIgnore = { "qstring.h" };
}

static string variableNameFromArg(Expr *arg)
{
    vector<DeclRefExpr*> declRefs;
    clazy::getChilds<DeclRefExpr>(arg, declRefs);
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

    auto method = dyn_cast<CXXMethodDecl>(func);
    if (!method || clazy::name(method) != "arg")
        return nullptr;

    CXXRecordDecl *record = method->getParent();
    if (!record || clazy::name(record) != "QString")
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
    if (clazy::classNameFor(secondParam) == "QString")
        return true;

    ParmVarDecl *firstParam = method->getParamDecl(0);
    if (clazy::classNameFor(firstParam) != "QString")
        return false;

    // This is a arg(QString, int, QChar) call, it's good if the second parameter is a default param
    return isa<CXXDefaultArgExpr>(callExpr->getArg(1));
}

bool QStringArg::checkMultiArgWarningCase(const vector<clang::CallExpr *> &calls)
{
    const int size = calls.size();
    for (int i = 1; i < size; ++i) {
        auto call = calls.at(i);
        if (calls.at(i - 1)->getNumArgs() + call->getNumArgs() <= 9) {
            emitWarning(getLocEnd(call), "Use multi-arg instead");
            return true;
        }
    }

    return false;
}

void QStringArg::checkForMultiArgOpportunities(CXXMemberCallExpr *memberCall)
{
    if (!isArgFuncWithOnlyQString(memberCall))
        return;

    if (getLocStart(memberCall).isMacroID()) {
        auto macroName = Lexer::getImmediateMacroName(getLocStart(memberCall), sm(), lo());
        if (macroName == "QT_REQUIRE_VERSION") // bug #391851
            return;
    }

    vector<clang::CallExpr *> callExprs = Utils::callListForChain(memberCall);
    vector<clang::CallExpr *> argCalls;
    for (auto call : callExprs) {
        if (!clazy::contains(m_alreadyProcessedChainedCalls, call) && isArgFuncWithOnlyQString(call)) {
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

void QStringArg::VisitStmt(clang::Stmt *stmt)
{
    auto memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall)
        return;

    if (shouldIgnoreFile(getLocStart(stmt)))
        return;

    checkForMultiArgOpportunities(memberCall);

    if (!isOptionSet("fillChar-overloads"))
        return;

    CXXMethodDecl *method = isArgMethod(memberCall->getDirectCallee());
    if (!method)
        return;

    if (clazy::simpleArgTypeName(method, method->getNumParams() - 1, lo()) == "QChar") {
        // The second arg wasn't passed, so this is a safe and unambiguous use, like .arg(1)
        if (isa<CXXDefaultArgExpr>(memberCall->getArg(1)))
            return;

        ParmVarDecl *p = method->getParamDecl(2);
        if (p && clazy::name(p) == "base") {
            // User went through the trouble specifying a base, lets allow it if it's a literal.
            vector<IntegerLiteral*> literals;
            clazy::getChilds<IntegerLiteral>(memberCall->getArg(2), literals);
            if (!literals.empty())
                return;

            string variableName = clazy::toLower(variableNameFromArg(memberCall->getArg(2)));
            if (clazy::contains(variableName, "base"))
                return;
        }

        p = method->getParamDecl(1);
        if (p && clazy::name(p) == "fieldWidth") {
            // He specified a literal, so he knows what he's doing, otherwise he would have put it directly in the string
            vector<IntegerLiteral*> literals;
            clazy::getChilds<IntegerLiteral>(memberCall->getArg(1), literals);
            if (!literals.empty())
                return;

            // the variable is named "width", user knows what he's doing
            string variableName = clazy::toLower(variableNameFromArg(memberCall->getArg(1)));
            if (clazy::contains(variableName, "width"))
                return;
        }

        emitWarning(getLocStart(stmt), "Using QString::arg() with fillChar overload");
    }
}
