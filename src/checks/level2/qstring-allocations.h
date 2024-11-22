/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_STRING_ALLOCATIONS_H
#define CLAZY_STRING_ALLOCATIONS_H

#include "checkbase.h"

#include <string>
#include <vector>

namespace clang
{
class FixItHint;
class CallExpr;
class StringLiteral;
class ConditionalOperator;
}

struct Latin1Expr;

enum FromFunction { FromLatin1, FromUtf8 };

/**
 * Finds places where there are unneeded memory allocations due to temporary QStrings.
 *
 * For example:
 * QString s = QLatin1String("foo"); // should be QStringLiteral
 * QString::fromLatin1("foo") and QString::fromUtf8("foo") // should be QStringLiteral, or QLatin1String if being passed to an overload taking QLatin1String
 *
 * See README-qstring-allocations for more information.
 */
class QStringAllocations : public CheckBase
{
public:
    QStringAllocations(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;

private:
    void VisitCtor(clang::Stmt *);
    void VisitCtor(clang::CXXConstructExpr *);
    void VisitOperatorCall(clang::Stmt *);
    void VisitFromLatin1OrUtf8(clang::Stmt *);
    void VisitAssignOperatorQLatin1String(clang::Stmt *);

    void maybeEmitWarning(clang::SourceLocation loc, std::string error, std::vector<clang::FixItHint> fixits = {});
    std::vector<clang::FixItHint> fixItReplaceWordWithWord(clang::Stmt *begin, const std::string &replacement, const std::string &replacee);
    std::vector<clang::FixItHint> fixItReplaceWordWithWordInTernary(clang::ConditionalOperator *);
    std::vector<clang::FixItHint> fixItReplaceFromLatin1OrFromUtf8(clang::CallExpr *callExpr, FromFunction);
    std::vector<clang::FixItHint> fixItRawLiteral(clang::StringLiteral *stmt, const std::string &replacement, clang::CXXOperatorCallExpr *operatorCall);

    Latin1Expr qlatin1CtorExpr(clang::Stmt *stm, clang::ConditionalOperator *&ternary);
};

#endif
