/*
    SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Waqar Ahmed <waqar.ahmed@kdab.com>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_USE_ARROW_OPERATOR_H
#define CLAZY_USE_ARROW_OPERATOR_H

#include "checkbase.h"

/**
 * See README-use-arrow-operator-instead-of-data.md for more info.
 */
class UseArrowOperatorInsteadOfData : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    void checkConnectArgQPointer(clang::Expr *arg);
};

#endif
