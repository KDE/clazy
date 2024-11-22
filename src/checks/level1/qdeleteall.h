/*
    SPDX-FileCopyrightText: 2015 Albert Astals Cid <albert.astals@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QDELETEALL_H
#define QDELETEALL_H

#include "checkbase.h"

#include <string>

/**
 * - QDeleteAll:
 *   - Finds places where you call qDeleteAll(set/map/hash.values()/keys())
 *
 *  See README-qdeleteall for more information
 */
class QDeleteAll : public CheckBase
{
public:
    QDeleteAll(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
