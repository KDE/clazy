/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_BASE_CLASS_EVENT_H
#define CLAZY_BASE_CLASS_EVENT_H

#include "checkbase.h"

#include <string>

/**
 * See README-base-class-event.md for more info.
 */
class BaseClassEvent : public CheckBase
{
public:
    explicit BaseClassEvent(const std::string &name);
    void VisitDecl(clang::Decl *) override;
};

#endif
