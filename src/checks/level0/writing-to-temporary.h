/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_WRITING_TO_TEMPORARY_H
#define CLANG_WRITING_TO_TEMPORARY_H

#include "checkbase.h"

#include <string>

/**
 * Finds places where writing to temporaries, which is a nop.
 *
 * See README-writing-to-temporary for more information
 */
class WritingToTemporary : public CheckBase
{
public:
    explicit WritingToTemporary(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
