/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DETACHING_MEMBER_H
#define DETACHING_MEMBER_H

#include "checks/detachingbase.h"

#include <string>

/**
 * Finds places where you're calling non-const member functions on member containers.
 *
 * For example m_list.first(), which would detach if the container is shared.
 * See README-deatching-member for more information
 */
class DetachingMember : public DetachingBase
{
public:
    explicit DetachingMember(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;
};

#endif
