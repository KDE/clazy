/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_VIRTUAL_SIGNAL_H
#define CLAZY_VIRTUAL_SIGNAL_H

#include "checkbase.h"

#include <string>

namespace clang
{
class Decl;
} // namespace clang

/**
 * See README-virtual-signal.md for more info.
 */
class VirtualSignal : public CheckBase
{
public:
    explicit VirtualSignal(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *stmt) override;

private:
};

#endif
