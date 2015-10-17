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

#include "qstringref.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <vector>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    FixitUseQStringRef = 0x1,
};

StringRefCandidates::StringRefCandidates(const std::string &name)
    : CheckBase(name)
{
}

static bool isInterestingFirstMethod(CXXMethodDecl *method)
{
    if (!method)
        return false;

    if (method->getParent()->getNameAsString() != "QString")
        return false;

    static const vector<string> list = { "left", "mid", "right" };
    return std::find(list.cbegin(), list.cend(), method->getNameAsString()) != list.cend();
}

static bool isInterestingSecondMethod(CXXMethodDecl *method)
{
    if (!method)
        return false;

    if (method->getParent()->getNameAsString() != "QString")
        return false;

    static const vector<string> list = { "compare", "contains", "count", "startsWith", "endsWith", "indexOf",
                                         "isEmpty", "isNull", "lastIndexOf", "length", "size", "toDouble", "toInt",
                                         "toUInt", "toULong", "toULongLong", "toUShort", "toUcs4"};
    const bool isInList = std::find(list.cbegin(), list.cend(), method->getNameAsString()) != list.cend();
    if (!isInList)
        return false;

    if (method->getNumParams() > 0) {
        // Check any argument is a QRegExp or QRegularExpression
        ParmVarDecl *firstParam = method->getParamDecl(0);
        if (firstParam) {
            const string paramSig = firstParam->getType().getAsString();
            if (paramSig == "const class QRegExp &" || paramSig == "class QRegExp &" || paramSig == "const class QRegularExpression &")
                return false;
        }
    }

    return true;
}

void StringRefCandidates::VisitStmt(clang::Stmt *stmt)
{
    // Here we look for code like str.firstMethod().secondMethod(), where firstMethod() is for example mid() and secondMethod is for example, toInt()

    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall)
        return;

    // In the AST secondMethod() is parent of firstMethod() call, and will be visited first (because at runtime firstMethod() is resolved first().
    // So check for interesting second method first
    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!isInterestingSecondMethod(method))
        return;

    vector<CallExpr *> callExprs = Utils::callListForChain(memberCall);
    if (callExprs.size() < 2)
        return;

    // The list now contains {secondMethod(), firstMethod() }
    CXXMemberCallExpr *firstMemberCall = dyn_cast<CXXMemberCallExpr>(callExprs.at(1));

    if (!firstMemberCall || !isInterestingFirstMethod(firstMemberCall->getMethodDecl()))
        return;
    const string firstMethodName = firstMemberCall->getMethodDecl()->getNameAsString();

    std::vector<FixItHint> fixits;
    if (isFixitEnabled(FixitUseQStringRef)) {
        fixits = fixit(firstMemberCall);
    }
    emitWarning(firstMemberCall->getLocEnd(), "Use " + firstMethodName + "Ref() instead", fixits);

}

std::vector<FixItHint> StringRefCandidates::fixit(CXXMemberCallExpr *call)
{
    MemberExpr *memberExpr = Utils::getFirstChildOfType<MemberExpr>(call);
    if (!memberExpr) {
        queueManualFixitWarning(call->getLocStart(), FixitUseQStringRef, "Internal error 1");
        return {};
    }

    auto insertionLoc = Lexer::getLocForEndOfToken(memberExpr->getLocEnd(), 0, m_ci.getSourceManager(), m_ci.getLangOpts());
    // llvm::errs() << insertionLoc.printToString(m_ci.getSourceManager()) << "\n";
    if (!insertionLoc.isValid()) {
        queueManualFixitWarning(call->getLocStart(), FixitUseQStringRef, "Internal error 2");
        return {};
    }

    std::vector<FixItHint> fixits;
    fixits.push_back(createInsertion(insertionLoc, "Ref"));
    return fixits;

}

const char *const s_checkName = "qstring-ref";
REGISTER_CHECK(s_checkName, StringRefCandidates)
REGISTER_FIXIT(FixitUseQStringRef, "fix-missing-qstringref", s_checkName)
