/*
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_TR_NON_LITERAL_H
#define CLAZY_TR_NON_LITERAL_H

#include "checkbase.h"

#include <string>

/**
 * See README-tr-non-literal.md for more info.
 */
class TrNonLiteral : public CheckBase
{
public:
    explicit TrNonLiteral(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
