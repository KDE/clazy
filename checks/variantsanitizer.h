/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#ifndef VARIANT_SANITIZER_H
#define VARIANT_SANITIZER_H

#include "checkbase.h"

/**
 * Detects when you're using QVariant::value<Foo>() instead of QVariant::toFoo().
 *
 * TODO: Missing QVariants of QHash<QString,QVariant>, QList<QVariant> and QMap<QString, QVariant>
 */
class VariantSanitizer : public CheckBase
{
public:
    VariantSanitizer(const std::string &name);
    void VisitStmt(clang::Stmt *stm) override;
};

#endif
