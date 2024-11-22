/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ASSERT_WITH_SIDE_EFFECTS_H
#define ASSERT_WITH_SIDE_EFFECTS_H

#include "checkbase.h"

#include <string>

class AssertWithSideEffects : public CheckBase
{
public:
    AssertWithSideEffects(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;

private:
    int m_aggressiveness;
};

#endif
