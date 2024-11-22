/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_EMPTY_QSTRINGLITERAL_H
#define CLAZY_EMPTY_QSTRINGLITERAL_H

#include "checkbase.h"

#include <string>

/**
 * See README-empty-qstringliteral.md for more info.
 */
class EmptyQStringliteral : public CheckBase
{
public:
    explicit EmptyQStringliteral(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;
    void handleQt6StringLiteral(clang::Stmt *);
    void handleQt5StringLiteral(clang::Stmt *);
};

#endif
