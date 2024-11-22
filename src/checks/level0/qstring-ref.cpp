/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-ref.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <array>
#include <vector>

using namespace clang;

StringRefCandidates::StringRefCandidates(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingFirstMethod(CXXMethodDecl *method)
{
    if (!method || clazy::name(method->getParent()) != "QString") {
        return false;
    }

    static const llvm::SmallVector<StringRef, 3> list{"left", "mid", "right"};
    return clazy::contains(list, clazy::name(method));
}

static bool isInterestingSecondMethod(CXXMethodDecl *method, const clang::LangOptions &lo)
{
    if (!method || clazy::name(method->getParent()) != "QString") {
        return false;
    }

    static const std::array<StringRef, 19> list{
        "compare", "contains", "count",   "startsWith", "endsWith", "indexOf", "isEmpty",     "isNull",   "lastIndexOf", "length",
        "size",    "toDouble", "toFloat", "toInt",      "toUInt",   "toULong", "toULongLong", "toUShort", "toUcs4",
    };

    if (!clazy::contains(list, clazy::name(method))) {
        return false;
    }

    return !clazy::anyArgIsOfAnySimpleType(method, {"QRegExp", "QRegularExpression"}, lo);
}

static bool isMethodReceivingQStringRef(CXXMethodDecl *method)
{
    if (!method || clazy::name(method->getParent()) != "QString") {
        return false;
    }

    static const std::array<StringRef, 8> list{"append", "compare", "count", "indexOf", "endsWith", "lastIndexOf", "localAwareCompare", "startsWidth"};

    if (clazy::contains(list, clazy::name(method))) {
        return true;
    }

    if (method->getOverloadedOperator() == OO_PlusEqual) { // operator+=
        return true;
    }

    return false;
}

void StringRefCandidates::VisitStmt(clang::Stmt *stmt)
{
    // Here we look for code like str.firstMethod().secondMethod(), where firstMethod() is for example mid() and secondMethod is for example, toInt()

    auto *call = dyn_cast<CallExpr>(stmt);
    if (!call || processCase1(dyn_cast<CXXMemberCallExpr>(call))) {
        return;
    }

    processCase2(call);
}

static bool containsChild(Stmt *s, Stmt *target)
{
    if (!s) {
        return false;
    }

    if (s == target) {
        return true;
    }

    if (auto *mte = dyn_cast<MaterializeTemporaryExpr>(s)) {
        return containsChild(mte->getSubExpr(), target);
    } else if (auto *ice = dyn_cast<ImplicitCastExpr>(s)) {
        return containsChild(ice->getSubExpr(), target);
    } else if (auto *bte = dyn_cast<CXXBindTemporaryExpr>(s)) {
        return containsChild(bte->getSubExpr(), target);
    }

    return false;
}

bool StringRefCandidates::isConvertedToSomethingElse(clang::Stmt *s) const
{
    // While passing a QString to the QVariant ctor works fine, passing QStringRef doesn't
    // So let's not warn when QStrings are cast to something else.
    if (!s) {
        return false;
    }

    auto *constr = clazy::getFirstParentOfType<CXXConstructExpr>(m_context->parentMap, s);
    if (!constr || constr->getNumArgs() == 0) {
        return false;
    }

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
    if (!memberCall) {
        return false;
    }

    // In the AST secondMethod() is parent of firstMethod() call, and will be visited first (because at runtime firstMethod() is resolved first().
    // So check for interesting second method first
    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!isInterestingSecondMethod(method, lo())) {
        return false;
    }

    std::vector<CallExpr *> callExprs = Utils::callListForChain(memberCall);
    if (callExprs.size() < 2) {
        return false;
    }

    // The list now contains {secondMethod(), firstMethod() }
    auto *firstMemberCall = dyn_cast<CXXMemberCallExpr>(callExprs.at(1));

    if (!firstMemberCall || !isInterestingFirstMethod(firstMemberCall->getMethodDecl())) {
        return false;
    }

    if (isConvertedToSomethingElse(memberCall)) {
        return false;
    }

    const std::string firstMethodName = firstMemberCall->getMethodDecl()->getNameAsString();
    const std::vector<FixItHint> fixits = fixit(firstMemberCall);

    emitWarning(firstMemberCall->getEndLoc(), "Use " + firstMethodName + "Ref() instead", fixits);
    return true;
}

// Catches cases like: s.append(s2.mid(1, 1));
bool StringRefCandidates::processCase2(CallExpr *call)
{
    auto *memberCall = dyn_cast<CXXMemberCallExpr>(call);
    auto *operatorCall = memberCall ? nullptr : dyn_cast<CXXOperatorCallExpr>(call);

    CXXMethodDecl *method = nullptr;
    if (memberCall) {
        method = memberCall->getMethodDecl();
    } else if (operatorCall && operatorCall->getCalleeDecl()) {
        Decl *decl = operatorCall->getCalleeDecl();
        method = dyn_cast<CXXMethodDecl>(decl);
    }

    if (!isMethodReceivingQStringRef(method)) {
        return false;
    }

    Expr *firstArgument = call->getNumArgs() > 0 ? call->getArg(0) : nullptr;
    MaterializeTemporaryExpr *temp = firstArgument ? dyn_cast<MaterializeTemporaryExpr>(firstArgument) : nullptr;
    if (!temp) {
        Expr *secondArgument = call->getNumArgs() > 1 ? call->getArg(1) : nullptr;
        temp = secondArgument ? dyn_cast<MaterializeTemporaryExpr>(secondArgument) : nullptr;
        if (!temp) { // For the CXXOperatorCallExpr it's in the second argument
            return false;
        }
    }

    auto *innerCall = clazy::getFirstChildOfType2<CallExpr>(temp);
    auto *innerMemberCall = innerCall ? dyn_cast<CXXMemberCallExpr>(innerCall) : nullptr;
    if (!innerMemberCall) {
        return false;
    }

    CXXMethodDecl *innerMethod = innerMemberCall->getMethodDecl();
    if (!isInterestingFirstMethod(innerMethod)) {
        return false;
    }

    const std::vector<FixItHint> fixits = fixit(innerMemberCall);

    emitWarning(call->getBeginLoc(), "Use " + innerMethod->getNameAsString() + "Ref() instead", fixits);
    return true;
}

std::vector<FixItHint> StringRefCandidates::fixit(CXXMemberCallExpr *call)
{
    auto *memberExpr = clazy::getFirstChildOfType<MemberExpr>(call);
    if (!memberExpr) {
        queueManualFixitWarning(call->getBeginLoc(), "Internal error 1");
        return {};
    }

    auto insertionLoc = Lexer::getLocForEndOfToken(memberExpr->getEndLoc(), 0, sm(), lo());
    // llvm::errs() << insertionLoc.printToString(sm()) << "\n";
    if (!insertionLoc.isValid()) {
        queueManualFixitWarning(call->getBeginLoc(), "Internal error 2");
        return {};
    }

    std::vector<FixItHint> fixits;
    fixits.push_back(clazy::createInsertion(insertionLoc, "Ref"));
    return fixits;
}
