/*
    SPDX-FileCopyrightText: 2020 Jesper K. Pedersen <jesper.pedersen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_USE_CHRONO_IN_QTIMER_H
#define CLAZY_USE_CHRONO_IN_QTIMER_H

#include "checkbase.h"

/**
 * See README-use-chrono-in-qtimer.md for more info.
 */
class UseChronoInQTimer : public CheckBase
{
public:
    using CheckBase::CheckBase;

    void VisitStmt(clang::Stmt *) override;

private:
    void warn(const clang::Stmt *stmt, int value);
    bool m_hasInsertedInclude = false;
};

#endif
