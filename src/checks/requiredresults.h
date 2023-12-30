/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef REQUIRED_RESULTS_H
#define REQUIRED_RESULTS_H

#include "checkbase.h"

#include <string>

/**
 * Warns if a result of a const member function is ignored.
 * There are lots of false positives. QDir::mkdir() for example.
 */
class RequiredResults : public CheckBase
{
public:
    RequiredResults(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;

private:
    bool shouldIgnoreMethod(const std::string &qualifiedName);
};

#endif
