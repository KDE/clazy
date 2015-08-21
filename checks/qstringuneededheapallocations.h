/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#ifndef MORE_WARNINGS_CAST_FROM_ASCII_TO_STRING_H
#define MORE_WARNINGS_CAST_FROM_ASCII_TO_STRING_H

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

    std::vector<clang::FixItHint> fixItReplaceWordWithWord(clang::Stmt *begin, const std::string &replacement, const std::string &replacee);
    std::vector<clang::FixItHint> fixItReplaceWordWithWordInTernary(clang::ConditionalOperator *);
    std::vector<clang::FixItHint> fixItReplaceFromLatin1OrFromUtf8(clang::CallExpr *callExpr);
    std::vector<clang::FixItHint> fixItRawLiteral(clang::StringLiteral *stmt, const std::string &replacement);
};

#endif
