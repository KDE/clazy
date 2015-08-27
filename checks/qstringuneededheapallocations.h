/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#ifndef CLANG_LAZY_CAST_FROM_ASCII_TO_STRING_H
#define CLANG_LAZY_CAST_FROM_ASCII_TO_STRING_H

#include "checkbase.h"

#include <map>
#include <vector>
#include <string>

namespace clang {
class FixItHint;
class ConditionalOperator;
class CallExpr;
class StringLiteral;
}

/**
 * Finds places where there are uneeded memory allocations due to temporary QStrings.
 *
 * For example:
 * QString s = QLatin1String("foo"); // should be QStringLiteral
 * QString::fromLatin1("foo") and QString::fromUtf8("foo") // should be QStringLiteral, or QLatin1String if being passed to an overload taking QLatin1String
 */
class QStringUneededHeapAllocations : public CheckBase
{
public:
    QStringUneededHeapAllocations(const std::string &name);
    void VisitStmt(clang::Stmt *stm) override;
private:
    void VisitCtor(clang::Stmt *);
    void VisitOperatorCall(clang::Stmt *);
    void VisitFromLatin1OrUtf8(clang::Stmt *);
    void VisitAssignOperatorQLatin1String(clang::Stmt *);

    std::vector<clang::FixItHint> fixItReplaceWordWithWord(clang::Stmt *begin, const std::string &replacement, const std::string &replacee, int fixitType);
    std::vector<clang::FixItHint> fixItReplaceWordWithWordInTernary(clang::ConditionalOperator *);
    std::vector<clang::FixItHint> fixItReplaceFromLatin1OrFromUtf8(clang::CallExpr *callExpr);
    std::vector<clang::FixItHint> fixItRawLiteral(clang::StringLiteral *stmt, const std::string &replacement);
    std::vector<clang::FixItHint> fixItRemoveToken(clang::Stmt *stmt, bool removeParenthesis);
};

#endif
