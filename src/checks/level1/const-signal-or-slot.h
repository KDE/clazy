/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONST_SIGNAL_OR_SLOT_H
#define CLAZY_CONST_SIGNAL_OR_SLOT_H

#include "checkbase.h"

/**
 * See README-const-signal-or-slot.md for more info.
 */
class ConstSignalOrSlot : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;
    void VisitDecl(clang::Decl *decl) override;
};

#endif
