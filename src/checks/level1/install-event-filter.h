/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_INSTALL_EVENT_FILTER_H
#define CLAZY_INSTALL_EVENT_FILTER_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * See README-install-event-filter.md for more info.
 */
class InstallEventFilter : public CheckBase
{
public:
    explicit InstallEventFilter(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
