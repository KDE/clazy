/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ASSERT_WITH_SIDE_EFFECTS_H
#define ASSERT_WITH_SIDE_EFFECTS_H

#include "checkbase.h"

class AssertWithSideEffects : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    enum Aggressiveness {
        NormalAggressiveness = 0,
        AlsoCheckFunctionCallsAggressiveness = 1 // too many false positives
    };
    Aggressiveness m_aggressiveness = NormalAggressiveness;
};

#endif
