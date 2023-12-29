/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_EMPTY_QSTRINGLITERAL_H
#define CLAZY_EMPTY_QSTRINGLITERAL_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class SourceLocation;
class Stmt;
} // namespace clang

/**
 * See README-empty-qstringliteral.md for more info.
 */
class EmptyQStringliteral : public CheckBase
{
public:
    explicit EmptyQStringliteral(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;
    bool maybeIgnoreUic(clang::SourceLocation) const;

private:
};

#endif
