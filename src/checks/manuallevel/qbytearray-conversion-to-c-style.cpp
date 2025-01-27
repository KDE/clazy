/*
    Copyright (C) 2025 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qbytearray-conversion-to-c-style.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

QBytearrayConversionToCStyle::QBytearrayConversionToCStyle(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QBytearrayConversionToCStyle::VisitStmt(clang::Stmt *stmt)
{
    if (!stmt)
        return;

    auto memberExpr = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberExpr) {
        return;
    }

    auto methodDecl = memberExpr->getMethodDecl();
    if (!methodDecl)
        return;
    CXXRecordDecl *className = methodDecl->getParent();
    if (!className || className->getName() != "QByteArray") {
        return;
    }

    constexpr const char byteArrayOp[] = "operator const char *";
    auto *callee = dyn_cast<MemberExpr>(memberExpr->getCallee());
    if (!callee || callee->getMemberNameInfo().getName().getAsString() != byteArrayOp) {
        return;
    }

    static const std::string msg = "Don't rely on the QByteArray implicit conversion to 'const char *'.";

    SourceRange sr = callee->getSourceRange();
    CharSourceRange cr = Lexer::getAsCharRange(sr, sm(), lo());
    std::string str = Lexer::getSourceText(cr, sm(), lo()).str();
    if (clazy::getFirstChildOfType<DeclRefExpr>(stmt)) { // E.g. QByteArray ba = "foo"
        std::vector<FixItHint> fixits = {clazy::createReplacement(sr, str + ".constData()")};
        emitWarning(sr.getEnd(), msg, fixits);
        return;
    }

    if (auto *funcCastExpr = clazy::getFirstChildOfType<CXXFunctionalCastExpr>(stmt)) {
        auto *literal = clazy::getFirstChildOfType<StringLiteral>(funcCastExpr);
        if (str.find("QByteArrayLiteral") != std::string_view::npos) { // QByteArrayLiteral("foo").
            // This only works with Qt6 where QByteArrayLiteral is a macro, but not with
            // Qt5 where QByteArrayLiteral is a lambda.
            std::string replacement = '"' + literal->getString().str() + '"'; // "foo"
            // The replacement has to be where the macro is expanded `QByteArrayLiteral("foo")`
            // not where it's defined.
            CharSourceRange expRange = sm().getExpansionRange(funcCastExpr->getBeginLoc());
            std::vector<FixItHint> fixits = {clazy::createReplacement(expRange.getAsRange(), replacement)};
            emitWarning(expRange.getBegin(), msg, fixits);
        } else if (str.find("QByteArray") != std::string_view::npos) { // QByteArray("foo")
            CharSourceRange cr = Lexer::getAsCharRange(literal->getSourceRange(), sm(), lo());
            std::string replacement = Lexer::getSourceText(cr, sm(), lo()).str(); // "foo"
            std::vector<FixItHint> fixits = {clazy::createReplacement(sr, replacement)};
            emitWarning(sr.getBegin(), msg, fixits);
        }
    }
}
