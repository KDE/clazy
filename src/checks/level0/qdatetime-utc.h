/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_LAZY_QDATETIME_UTC_H
#define CLANG_LAZY_QDATETIME_UTC_H

#include "checkbase.h"

/**
 * Finds expensive calls to QDateTime::currentDateTime().
 *
 * See README-qdatetime-utc for more information.
 */
class QDateTimeUtc : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
