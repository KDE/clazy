/*
   This file is part of the clang-lazy static checker.

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

#include "qstringref.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "FixItUtils.h"

#include <clang/AST/AST.h>
#include <vector>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    FixitUseQStringRef = 0x1,
};

StringRefCandidates::StringRefCandidates(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
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

static bool isMethodReceivingQStringRef(CXXMethodDecl *method)
{
    static const vector<string> list = { "append", "compare", "count", "indexOf", "endsWith", "lastIndexOf", "localAwareCompare", "startsWidth", "operator+=" };

    if (!method || method->getParent()->getNameAsString() != "QString")
        return false;

    return find(list.cbegin(), list.cend(), method->getNameAsString()) != list.cend();
}

void StringRefCandidates::VisitStmt(clang::Stmt *stmt)
{
    // Here we look for code like str.firstMethod().secondMethod(), where firstMethod() is for example mid() and secondMethod is for example, toInt()

    CallExpr *call = dyn_cast<CallExpr>(stmt);
    if (!call)
        return;

    if (processCase1(dyn_cast<CXXMemberCallExpr>(call)))
        return;

    processCase2(call);
}

// Catches cases like: int i = s.mid(1, 1).toInt()
bool StringRefCandidates::processCase1(CXXMemberCallExpr *memberCall)
{
    if (!memberCall)
        return false;

    // In the AST secondMethod() is parent of firstMethod() call, and will be visited first (because at runtime firstMethod() is resolved first().
    // So check for interesting second method first
    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!isInterestingSecondMethod(method))
        return false;

    vector<CallExpr *> callExprs = Utils::callListForChain(memberCall);
    if (callExprs.size() < 2)
        return false;

    // The list now contains {secondMethod(), firstMethod() }
    CXXMemberCallExpr *firstMemberCall = dyn_cast<CXXMemberCallExpr>(callExprs.at(1));

    if (!firstMemberCall || !isInterestingFirstMethod(firstMemberCall->getMethodDecl()))
        return false;
    const string firstMethodName = firstMemberCall->getMethodDecl()->getNameAsString();

    std::vector<FixItHint> fixits;
    if (isFixitEnabled(FixitUseQStringRef)) {
        fixits = fixit(firstMemberCall);
    }
    emitWarning(firstMemberCall->getLocEnd(), "Use " + firstMethodName + "Ref() instead", fixits);
    return true;
}

// Catches cases like: s.append(s2.mid(1, 1));
bool StringRefCandidates::processCase2(CallExpr *call)
{
    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(call);
    CXXOperatorCallExpr *operatorCall = dyn_cast<CXXOperatorCallExpr>(call);

    CXXMethodDecl *method = nullptr;
    if (memberCall) {
        method = memberCall->getMethodDecl();
    } else if (operatorCall && operatorCall->getCalleeDecl()) {
        Decl *decl = operatorCall->getCalleeDecl();
        method = dyn_cast<CXXMethodDecl>(decl);
    }

    if (!isMethodReceivingQStringRef(method))
        return false;

    Expr *firstArgument = call->getNumArgs() > 0 ? call->getArg(0) : nullptr;
    MaterializeTemporaryExpr *temp = firstArgument ? dyn_cast<MaterializeTemporaryExpr>(firstArgument) : nullptr;
    if (!temp) {
        Expr *secondArgument = call->getNumArgs() > 1 ? call->getArg(1) : nullptr;
        temp = secondArgument ? dyn_cast<MaterializeTemporaryExpr>(secondArgument) : nullptr;
        if (!temp) // For the CXXOperatorCallExpr it's in the second argument
            return false;
    }

    CallExpr *innerCall = HierarchyUtils::getFirstChildOfType2<CallExpr>(temp);
    CXXMemberCallExpr *innerMemberCall = innerCall ? dyn_cast<CXXMemberCallExpr>(innerCall) : nullptr;
    if (!innerMemberCall)
        return false;

    CXXMethodDecl *innerMethod = innerMemberCall->getMethodDecl();
    if (!isInterestingFirstMethod(innerMethod))
        return false;

    std::vector<FixItHint> fixits;
    if (isFixitEnabled(FixitUseQStringRef)) {
        fixits = fixit(innerMemberCall);
    }

    emitWarning(call->getLocStart(), "Use " + innerMethod->getNameAsString() + "Ref() instead", fixits);
    return true;
}

std::vector<FixItHint> StringRefCandidates::fixit(CXXMemberCallExpr *call)
{
    MemberExpr *memberExpr = HierarchyUtils::getFirstChildOfType<MemberExpr>(call);
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
    fixits.push_back(FixItUtils::createInsertion(insertionLoc, "Ref"));
    return fixits;

}

const char *const s_checkName = "qstring-ref";
REGISTER_CHECK_WITH_FLAGS(s_checkName, StringRefCandidates, CheckLevel0)
REGISTER_FIXIT(FixitUseQStringRef, "fix-missing-qstringref", s_checkName)
