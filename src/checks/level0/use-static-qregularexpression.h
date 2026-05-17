/*
    SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Waqar Ahmed <waqar.ahmed@kdab.com>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_USE_STATIC_QREGULAREXPRESSION_H
#define CLAZY_USE_STATIC_QREGULAREXPRESSION_H

#include "checkbase.h"

/**
 * See README-use-static-qregularexpression.md for more info.
 */
class UseStaticQRegularExpression : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    bool isTemporaryQRegexObj(clang::Expr *qregexVar);
    bool firstArgIsQRegularExpression(clang::CXXMethodDecl *methodDecl);
};

#endif
