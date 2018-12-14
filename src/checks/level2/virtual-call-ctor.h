/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015 Sergio Martins <smartins@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef VIRTUALCALLSFROMCTOR_H
#define VIRTUALCALLSFROMCTOR_H

#include "checkbase.h"

#include <vector>
#include <string>

class ClazyContext;

namespace clang {
class CXXRecordDecl;
class Stmt;
class SourceLocation;
class Decl;
}

/**
 * Finds places where you're calling pure virtual functions inside a CTOR or DTOR.
 * Compilers warn about this if there isn't any indirection, this plugin will catch cases like calling
 * a non-pure virtual that calls a pure virtual.
 *
 * This plugin only checks for pure virtuals, ignoring non-pure, which in theory you shouldn't call,
 * but seems common practice.
 */
class VirtualCallCtor
    : public CheckBase
{
public:
    VirtualCallCtor(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
    clang::SourceLocation containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt,
                                              std::vector<clang::Stmt*> &processedStmts);
};


#endif
