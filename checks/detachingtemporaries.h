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

#ifndef DETACHING_TEMPORARIES_H
#define DETACHING_TEMPORARIES_H

#include "checkbase.h"

#include <map>
#include <vector>
#include <string>

/**
 * Finds places where you're calling non-const member functions on temporaries.
 *
 * For example getList().first(), which would detach if the container is shared.
 *
 * TODO: Missing operator[]
 * Probability of False-Positives: yes, for example someHash.values().first() doesn't detach
 * because refcount is 1. But should be constFirst() anyway.
 */
class DetachingTemporaries : public CheckBase
{
public:
    DetachingTemporaries(const std::string &name);
    void VisitStmt(clang::Stmt *stm) override;
private:
    std::map<std::string, std::vector<std::string>> m_methodsByType;
};

#endif
