/*
    SPDX-FileCopyrightText: 2026 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_OPERATOR_BRACKETS_INSERTION_H
#define CLAZY_OPERATOR_BRACKETS_INSERTION_H

#include "checkbase.h"

/**
 * See README-operator-brackets-insertion.md for more info.
 */
class OperatorBracketsInsertion : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    bool isAssociativeContainerOperatorSubscript(const clang::CXXOperatorCallExpr *op) const;
};

#endif
