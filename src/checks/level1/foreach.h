/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FOREACH_DETACHMENTS_H
#define FOREACH_DETACHMENTS_H

#include "checkbase.h"

#include <string>

namespace clang
{
class ForStmt;
class ValueDecl;
}

/**
 * - Foreach:
 *   - Finds places where you're detaching the foreach container.
 *   - Finds places where big or non-trivial types are passed by value instead of const-ref.
 *   - Finds places where you're using foreach on STL containers. It causes deep-copy.
 * - For Range Loops:
 *   - Finds places where you're using C++11 for range loops with Qt containers. (potential detach)
 */
class Foreach : public CheckBase
{
public:
    Foreach(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    void checkBigTypeMissingRef();
    bool containsDetachments(clang::Stmt *stmt, clang::ValueDecl *containerValueDecl);
    clang::ForStmt *m_lastForStmt = nullptr;
};

#endif
