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

#include "qstringref.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "HierarchyUtils.h"
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

StringRefCandidates::StringRefCandidates(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingFirstMethod(CXXMethodDecl *method)
{
    if (!method || method->getParent()->getNameAsString() != "QString")
        return false;

    static const vector<string> list = { "left", "mid", "right" };
    return clazy_std::contains(list, method->getNameAsString());
}

static bool isInterestingSecondMethod(CXXMethodDecl *method, const clang::LangOptions &lo)
{
    if (!method || method->getParent()->getNameAsString() != "QString")
        return false;

    static const vector<string> list = { "compare", "contains", "count", "startsWith", "endsWith", "indexOf",
                                         "isEmpty", "isNull", "lastIndexOf", "length", "size", "toDouble", "toFloat",
                                         "toInt", "toUInt", "toULong", "toULongLong", "toUShort", "toUcs4" };

    if (!clazy_std::contains(list, method->getNameAsString()))
        return false;

    return !StringUtils::anyArgIsOfAnySimpleType(method, {"QRegExp", "QRegularExpression"}, lo);
}

static bool isMethodReceivingQStringRef(CXXMethodDecl *method)
{
    static const vector<string> list = { "append", "compare", "count", "indexOf", "endsWith", "lastIndexOf", "localAwareCompare", "startsWidth", "operator+=" };

    if (!method || method->getParent()->getNameAsString() != "QString")
        return false;

    return clazy_std::contains(list, method->getNameAsString());
}

void StringRefCandidates::VisitStmt(clang::Stmt *stmt)
{
    // Here we look for code like str.firstMethod().secondMethod(), where firstMethod() is for example mid() and secondMethod is for example, toInt()

    auto call = dyn_cast<CallExpr>(stmt);
    if (!call || processCase1(dyn_cast<CXXMemberCallExpr>(call)))
        return;

    processCase2(call);
}

static bool containsChild(Stmt *s, Stmt *target)
{
    if (!s)
        return false;

    if (s == target)
        return true;

    if (auto mte = dyn_cast<MaterializeTemporaryExpr>(s)) {
        return containsChild(mte->getTemporary(), target);
    } else if (auto ice = dyn_cast<ImplicitCastExpr>(s)) {
        return containsChild(ice->getSubExpr(), target);
    } else if (auto bte = dyn_cast<CXXBindTemporaryExpr>(s)) {
        return containsChild(bte->getSubExpr(), target);
    }

    return false;
}

bool StringRefCandidates::isConvertedToSomethingElse(clang::Stmt* s) const
{
    // While passing a QString to the QVariant ctor works fine, passing QStringRef doesn't
    // So let's not warn when QStrings are cast to something else.
    if (!s)
        return false;

    auto constr = HierarchyUtils::getFirstParentOfType<CXXConstructExpr>(m_context->parentMap, s);
    if (!constr || constr->getNumArgs() == 0)
        return false;

    if (containsChild(constr->getArg(0), s)) {
        CXXConstructorDecl *ctor = constr->getConstructor();
        CXXRecordDecl *record = ctor ? ctor->getParent() : nullptr;
        return record ? record->getQualifiedNameAsString() != "QString" : false;

    }

    return false;
}

// Catches cases like: int i = s.mid(1, 1).toInt()
bool StringRefCandidates::processCase1(CXXMemberCallExpr *memberCall)
{
    if (!memberCall)
        return false;

    // In the AST secondMethod() is parent of firstMethod() call, and will be visited first (because at runtime firstMethod() is resolved first().
    // So check for interesting second method first
    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!isInterestingSecondMethod(method, lo()))
        return false;

    vector<CallExpr *> callExprs = Utils::callListForChain(memberCall);
    if (callExprs.size() < 2)
        return false;

    // The list now contains {secondMethod(), firstMethod() }
    auto firstMemberCall = dyn_cast<CXXMemberCallExpr>(callExprs.at(1));

    if (!firstMemberCall || !isInterestingFirstMethod(firstMemberCall->getMethodDecl()))
        return false;

    if (isConvertedToSomethingElse(memberCall))
        return false;

    const string firstMethodName = firstMemberCall->getMethodDecl()->getNameAsString();
    std::vector<FixItHint> fixits;
    if (isFixitEnabled(FixitUseQStringRef))
        fixits = fixit(firstMemberCall);

    emitWarning(firstMemberCall->getLocEnd(), "Use " + firstMethodName + "Ref() instead", fixits);
    return true;
}

// Catches cases like: s.append(s2.mid(1, 1));
bool StringRefCandidates::processCase2(CallExpr *call)
{
    auto memberCall = dyn_cast<CXXMemberCallExpr>(call);
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
    auto innerMemberCall = innerCall ? dyn_cast<CXXMemberCallExpr>(innerCall) : nullptr;
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

    auto insertionLoc = Lexer::getLocForEndOfToken(memberExpr->getLocEnd(), 0, sm(), lo());
    // llvm::errs() << insertionLoc.printToString(sm()) << "\n";
    if (!insertionLoc.isValid()) {
        queueManualFixitWarning(call->getLocStart(), FixitUseQStringRef, "Internal error 2");
        return {};
    }

    std::vector<FixItHint> fixits;
    fixits.push_back(FixItUtils::createInsertion(insertionLoc, "Ref"));
    return fixits;

}
