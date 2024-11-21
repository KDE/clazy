/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-arg.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "PreProcessorVisitor.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;

QStringArg::QStringArg(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    m_filesToIgnore = {"qstring.h"};
    context->enablePreprocessorVisitor();
}

static std::string variableNameFromArg(Expr *arg)
{
    std::vector<DeclRefExpr *> declRefs;
    clazy::getChilds<DeclRefExpr>(arg, declRefs);
    if (declRefs.size() == 1) {
        ValueDecl *decl = declRefs.at(0)->getDecl();
        return decl ? decl->getNameAsString() : std::string();
    }

    return {};
}

static CXXMethodDecl *isArgMethod(FunctionDecl *func, const char *className)
{
    if (!func) {
        return nullptr;
    }

    auto *method = dyn_cast<CXXMethodDecl>(func);
    if (!method || clazy::name(method) != "arg") {
        return nullptr;
    }

    const CXXRecordDecl *record = method->getParent();
    if (!record || clazy::name(record) != className) {
        return nullptr;
    }

    return method;
}

static bool isArgFuncWithOnlyQString(CallExpr *callExpr)
{
    if (!callExpr) {
        return false;
    }

    CXXMethodDecl *method = isArgMethod(callExpr->getDirectCallee(), "QString");
    if (!method) {
        return false;
    }

    ParmVarDecl *secondParam = method->getParamDecl(1);
    if (clazy::classNameFor(secondParam) == "QString") {
        return true;
    }

    ParmVarDecl *firstParam = method->getParamDecl(0);
    if (clazy::classNameFor(firstParam) != "QString") {
        return false;
    }

    // This is a arg(QString, int, QChar) call, it's good if the second parameter is a default param
    return isa<CXXDefaultArgExpr>(callExpr->getArg(1));
}

bool QStringArg::checkMultiArgWarningCase(const std::vector<clang::CallExpr *> &calls)
{
    if (calls.size() == 1) {
        return false; // Nothing to do
    }
    std::string replacement;
    SourceLocation beginLoc;
    CallExpr *call = nullptr;
    int argAggregated = 0;
    for (int i = 0, size = calls.size(); i < size; ++i) {
        call = calls.at(i);
        for (auto *arg : call->arguments()) {
            if (!isa<CXXDefaultArgExpr>(arg)) {
                ++argAggregated;
            }
        }
        if (argAggregated > 9) { // Relevant for Qt5
            return false;
        }
        if (!beginLoc.isValid()) {
            beginLoc = call->getBeginLoc();
        }

        std::string callArgs;
        for (auto *arg : call->arguments()) {
            if (!isa<CXXDefaultArgExpr>(arg)) {
                if (!callArgs.empty()) {
                    callArgs += ", ";
                }
                callArgs += Lexer::getSourceText(CharSourceRange::getTokenRange(arg->getSourceRange()), sm(), lo()).str();
            }
        }
        // The args for the chained calls have to be prepended instead of appended
        replacement = callArgs + (replacement.empty() ? "" : ", ") + replacement;
    }
    if (auto *subexprCall = clazy::getFirstChildOfType<MemberExpr>(call)) {
        emitWarning(beginLoc,
                    "Use multi-arg instead",
                    {FixItHint::CreateReplacement(SourceRange(subexprCall->getEndLoc(), calls.at(0)->getEndLoc()), "arg(" + replacement + ")")});
    }

    return false;
}

void QStringArg::checkForMultiArgOpportunities(CXXMemberCallExpr *memberCall)
{
    if (!isArgFuncWithOnlyQString(memberCall)) {
        return;
    }

    if (memberCall->getBeginLoc().isMacroID()) {
        auto macroName = Lexer::getImmediateMacroName(memberCall->getBeginLoc(), sm(), lo());
        if (macroName == "QT_REQUIRE_VERSION") { // bug #391851
            return;
        }
    }

    std::vector<clang::CallExpr *> callExprs = Utils::callListForChain(memberCall);
    std::vector<clang::CallExpr *> argCalls;
    for (auto *call : callExprs) {
        if (!clazy::contains(m_alreadyProcessedChainedCalls, call) && isArgFuncWithOnlyQString(call)) {
            argCalls.push_back(call);
            m_alreadyProcessedChainedCalls.push_back(call);
        } else {
            if (checkMultiArgWarningCase(argCalls)) {
                return;
            }
            argCalls.clear();
        }
    }

    checkMultiArgWarningCase(argCalls);
}

bool QStringArg::checkQLatin1StringCase(CXXMemberCallExpr *memberCall)
{
    const PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
    if (!preProcessorVisitor || preProcessorVisitor->qtVersion() < 51400) {
        // QLatin1String::arg() was introduced in Qt 5.14
        return false;
    }

    if (!isArgMethod(memberCall->getDirectCallee(), "QLatin1String")) {
        return false;
    }

    if (memberCall->getNumArgs() == 0) {
        return false;
    }

    Expr *arg = memberCall->getArg(0);
    QualType t = arg->getType();
    if (!t->isIntegerType() || t->isCharType()) {
        return false;
    }

    emitWarning(memberCall, "Argument passed to QLatin1String::arg() will be implicitly cast to QChar");
    return true;
}

void QStringArg::VisitStmt(clang::Stmt *stmt)
{
    auto *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall) {
        return;
    }

    if (shouldIgnoreFile(stmt->getBeginLoc())) {
        return;
    }

    checkForMultiArgOpportunities(memberCall);

    if (checkQLatin1StringCase(memberCall)) {
        return;
    }

    if (!isOptionSet("fillChar-overloads")) {
        return;
    }

    CXXMethodDecl *method = isArgMethod(memberCall->getDirectCallee(), "QString");
    if (!method) {
        return;
    }

    if (clazy::simpleArgTypeName(method, method->getNumParams() - 1, lo()) == "QChar") {
        // The second arg wasn't passed, so this is a safe and unambiguous use, like .arg(1)
        if (isa<CXXDefaultArgExpr>(memberCall->getArg(1))) {
            return;
        }

        const ParmVarDecl *p = method->getParamDecl(2);
        if (p && clazy::name(p) == "base") {
            // User went through the trouble specifying a base, lets allow it if it's a literal.
            std::vector<IntegerLiteral *> literals;
            clazy::getChilds<IntegerLiteral>(memberCall->getArg(2), literals);
            if (!literals.empty()) {
                return;
            }

            std::string variableName = clazy::toLower(variableNameFromArg(memberCall->getArg(2)));
            if (clazy::contains(variableName, "base")) {
                return;
            }
        }

        p = method->getParamDecl(1);
        if (p && clazy::name(p) == "fieldWidth") {
            // He specified a literal, so he knows what he's doing, otherwise he would have put it directly in the string
            std::vector<IntegerLiteral *> literals;
            clazy::getChilds<IntegerLiteral>(memberCall->getArg(1), literals);
            if (!literals.empty()) {
                return;
            }

            // the variable is named "width", user knows what he's doing
            std::string variableName = clazy::toLower(variableNameFromArg(memberCall->getArg(1)));
            if (clazy::contains(variableName, "width")) {
                return;
            }
        }

        emitWarning(stmt->getBeginLoc(), "Using QString::arg() with fillChar overload");
    }
}
