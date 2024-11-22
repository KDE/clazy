/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_WRONG_QEVENT_CAST_H
#define CLAZY_WRONG_QEVENT_CAST_H

#include "checkbase.h"

#include <string>

/**
 * See README-wrong-qevent-cast.md for more info.
 */
class WrongQEventCast : public CheckBase
{
public:
    explicit WrongQEventCast(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;

private:
};

#endif
