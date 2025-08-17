/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONNECT_BY_NAME_H
#define CLAZY_CONNECT_BY_NAME_H

#include "checkbase.h"

/**
 * See README-connect-by-name.md for more info.
 */
class ConnectByName : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *decl) override;
};

#endif
