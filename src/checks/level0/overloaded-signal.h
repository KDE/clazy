/*
  This file is part of the clazy static checker.

  SPDX-FileCopyrightText: 2019 Sergio Martins <smartins@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_OVERLOADED_SIGNAL_H
#define CLAZY_OVERLOADED_SIGNAL_H

#include "checkbase.h"

/**
 * See README-overloaded-signal.md for more info.
 */
class OverloadedSignal : public CheckBase
{
public:
    explicit OverloadedSignal(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;
};

#endif
