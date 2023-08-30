/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_POST_EVENT_H
#define CLAZY_POST_EVENT_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * See README-post-event for more info.
 */
class PostEvent : public CheckBase
{
public:
    explicit PostEvent(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
