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
    explicit RequiredResults(clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stm) override;
    std::string name() const override;
private:
    bool shouldIgnoreMethod(const std::string &qualifiedName);
};

#endif
