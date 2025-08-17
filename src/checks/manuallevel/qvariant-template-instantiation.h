/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef VARIANT_SANITIZER_H
#define VARIANT_SANITIZER_H

#include "checkbase.h"

/**
 * Detects when you're using QVariant::value<Foo>() instead of QVariant::toFoo().
 *
 */
class QVariantTemplateInstantiation : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stm) override;
};

#endif
