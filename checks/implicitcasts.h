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

#ifndef MORE_WARNINGS_IMPLICIT_CASTS_H
#define MORE_WARNINGS_IMPLICIT_CASTS_H

#include "checkbase.h"

namespace clang {
class ForStmt;
class ValueDecl;
class Stmt;
}

/**
 * Finds places with unwanted implicit casts
 */
class ImplicitCasts : public CheckBase
{
public:
    ImplicitCasts(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
protected:
    std::vector<std::string> filesToIgnore() const override;
};

#endif
