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
    explicit QStringUneededHeapAllocations(clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stm) override;
    std::string name() const override;
private:
    void VisitCtor(clang::Stmt *);
    void VisitOperatorCall(clang::Stmt *);
    void VisitFromLatin1OrUtf8(clang::Stmt *);
    void VisitAssignOperatorQLatin1String(clang::Stmt *);
};

#endif
