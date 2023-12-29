/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef NON_POD_STATIC_H
#define NON_POD_STATIC_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * Finds global static non-POD variables.
 *
 * See README-non-pod-global-static.
 */
class NonPodGlobalStatic : public CheckBase
{
public:
    explicit NonPodGlobalStatic(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;
};

#endif
