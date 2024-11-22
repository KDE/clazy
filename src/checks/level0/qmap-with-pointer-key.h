/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QMAP_WITH_POINTER_KEY_H
#define QMAP_WITH_POINTER_KEY_H

#include "checkbase.h"

#include <string>

namespace clang
{
class Decl;
} // namespace clang

/**
 * Finds cases where you're using QMap<K,T> and K is a pointer. QHash<K,T> should be used instead.
 *
 * See README-qmap-with-pointer-key for more information.
 */
class QMapWithPointerKey : public CheckBase
{
public:
    QMapWithPointerKey(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
};

#endif
