/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

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

#ifndef CLAZY_STRING_ALLOCATIONS_H
#define CLAZY_STRING_ALLOCATIONS_H

#include "checkbase.h"

#include <map>
#include <vector>
#include <string>

class ClazyContext;

namespace clang {
class FixItHint;
class ConditionalOperator;
class CallExpr;
class StringLiteral;
class ConditionalOperator;
class Stmt;
}

struct Latin1Expr;

enum FromFunction {
    FromLatin1,
    FromUtf8
};


/**
 * Finds places where there are unneeded memory allocations due to temporary QStrings.
 *
 * For example:
 * QString s = QLatin1String("foo"); // should be QStringLiteral
 * QString::fromLatin1("foo") and QString::fromUtf8("foo") // should be QStringLiteral, or QLatin1String if being passed to an overload taking QLatin1String
 *
 * See README-qstring-allocations for more information.
 */
class QStringAllocations
    : public CheckBase
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
    std::vector<clang::FixItHint> fixItRawLiteral(clang::StringLiteral *stmt, const std::string &replacement);

    Latin1Expr qlatin1CtorExpr(clang::Stmt *stm, clang::ConditionalOperator * &ternary);
};

#endif
