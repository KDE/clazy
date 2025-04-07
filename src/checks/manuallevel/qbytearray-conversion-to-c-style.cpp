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

// #include <iostream>

using namespace clang;

QBytearrayConversionToCStyle::QBytearrayConversionToCStyle(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QBytearrayConversionToCStyle::VisitStmt(clang::Stmt *stmt)
{
    if (!stmt) {
        return;
    }

    auto memberExpr = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberExpr) {
        return;
    }

    auto methodDecl = memberExpr->getMethodDecl();
    if (!methodDecl) {
        return;
    }
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
    if (!sr.isValid()) {
        return;
    }

    if (auto *funcCastExpr = clazy::getFirstChildOfType<CXXFunctionalCastExpr>(stmt)) {
        auto *literal = clazy::getFirstChildOfType<StringLiteral>(funcCastExpr);
        if (!literal) {
            return;
        }
        CharSourceRange cr = Lexer::getAsCharRange(sr, sm(), lo());
        if (!cr.isValid()) {
            return;
        }
        std::string_view str = Lexer::getSourceText(cr, sm(), lo());
        // QByteArrayLiteral("foo") or QByteArray("foo")
        if (str.back() != ')') {
            return;
        }

        auto modifyStr = [&str](const char *name) {
            str.remove_suffix(1);
            str.remove_prefix(strlen(name));
        };
        // This only works with Qt6 where QByteArrayLiteral is a macro, but not with
        // Qt5 where QByteArrayLiteral is a lambda.
        if (str.find("QByteArrayLiteral(") == 0) {
            modifyStr("QByteArrayLiteral(");
            CharSourceRange expRange = sm().getExpansionRange(funcCastExpr->getBeginLoc());
            if (!expRange.isValid()) {
                return;
            }
            std::vector<FixItHint> fixits = {clazy::createReplacement(expRange.getAsRange(), std::string{str})};
            emitWarning(expRange.getBegin(), msg, fixits);
            return;
        } else if (str.find("QByteArray(") == 0) {
            modifyStr("QByteArray(");
            std::vector<FixItHint> fixits = {clazy::createReplacement(sr, std::string{str})};
            emitWarning(sr.getBegin(), msg, fixits);
            return;
        }
    }

    SourceLocation begin = sm().getSpellingLoc(sr.getBegin());
    SourceLocation end = sm().getSpellingLoc(sr.getEnd());
    CharSourceRange r = CharSourceRange::getTokenRange(begin, end);
    if (!r.isValid()) {
        return;
    }

    StringRef text = Lexer::getSourceText(r, sm(), lo());
    if (text.empty()) {
        return;
    }

    if (auto *opCallExpr = clazy::getFirstChildOfType<CXXOperatorCallExpr>(stmt)) {
        if (opCallExpr->getOperator() == clang::OO_Plus) {
            std::string fixed = "QByteArray{" + std::string{text} + "}.constData()";
            emitWarning(begin, msg, {clazy::createReplacement(sr, fixed)});
            return;
        }
    }

    std::vector<FixItHint> fixits = {clazy::createReplacement(sr, std::string{text} + ".constData()")};
    emitWarning(begin, msg, fixits);
}
