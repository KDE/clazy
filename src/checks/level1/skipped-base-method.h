/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_SKIPPED_BASE_METHOD_H
#define CLAZY_SKIPPED_BASE_METHOD_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * See README-skipped-base-method.md for more info.
 */
class SkippedBaseMethod : public CheckBase
{
public:
    explicit SkippedBaseMethod(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
