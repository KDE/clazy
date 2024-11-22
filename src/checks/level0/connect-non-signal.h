/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2016 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONNECT_NON_SIGNAL_H
#define CLAZY_CONNECT_NON_SIGNAL_H

#include "checkbase.h"

#include <string>

/**
 * See README-connect-non-signal.md for more info.
 */
class ConnectNonSignal : public CheckBase
{
public:
    explicit ConnectNonSignal(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
