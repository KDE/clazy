/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_OVERRIDDEN_SIGNAL_H
#define CLAZY_OVERRIDDEN_SIGNAL_H

#include "checkbase.h"

#include <string>

/**
 * See README-overridden-signal.md for more info.
 */
class OverriddenSignal : public CheckBase
{
public:
    explicit OverriddenSignal(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
};

#endif
