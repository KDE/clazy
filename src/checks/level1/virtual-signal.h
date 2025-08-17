/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_VIRTUAL_SIGNAL_H
#define CLAZY_VIRTUAL_SIGNAL_H

#include "checkbase.h"

/**
 * See README-virtual-signal.md for more info.
 */
class VirtualSignal : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *stmt) override;

private:
};

#endif
