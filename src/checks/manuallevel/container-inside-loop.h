/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONTAINER_INSIDE_LOOP_H
#define CLAZY_CONTAINER_INSIDE_LOOP_H

#include "checkbase.h"

#include <string>

/**
 * Finds places defining containers inside loops. Defining them outside and using resize(0) will
 * save allocations.
 *
 * See README-container-inside-loop for more information
 */
class ContainerInsideLoop : public CheckBase
{
public:
    explicit ContainerInsideLoop(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
