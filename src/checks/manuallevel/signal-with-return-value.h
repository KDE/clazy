/*
    SPDX-FileCopyrightText: 2019 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sergio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_SIGNAL_WITH_RETURN_VALUE_H
#define CLAZY_SIGNAL_WITH_RETURN_VALUE_H

#include "checkbase.h"

/**
 * See README-signal-with-return-value.md for more info.
 */
class SignalWithReturnValue : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *) override;

private:
};

#endif
