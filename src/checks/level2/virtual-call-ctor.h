/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef VIRTUALCALLSFROMCTOR_H
#define VIRTUALCALLSFROMCTOR_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
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
class VirtualCallCtor : public CheckBase
{
public:
    VirtualCallCtor(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
    clang::SourceLocation containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<clang::Stmt *> &processedStmts);
};

#endif
