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

#ifndef VIRTUALCALLSFROMCTOR_H
#define VIRTUALCALLSFROMCTOR_H

#include "checkbase.h"

#include <vector>

namespace clang {
class CXXRecordDecl;
class Stmt;
class SourceLocation;
}

/**
 * Finds places where you're calling pure virtual functions inside a CTOR or DTOR.
 * Compilers warn about this if there isn't any indirection, this plugin will catch cases like calling
 * a non-pure virtual that calls a pure virtual.
 *
 * This plugin only checks for pure virtuals, ignoring non-pure, which in theory you shouldn't call,
 * but seems common practice.
 */
class VirtualCallsFromCTOR : public CheckBase
{
public:
    VirtualCallsFromCTOR(const std::string &name);
    void VisitStmt(clang::Stmt *stm) override;
    void VisitDecl(clang::Decl *decl) override;

private:
    bool containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<clang::Stmt*> &processedStmts) const;
};


#endif
