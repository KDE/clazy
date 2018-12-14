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

#ifndef CLANG_LAZY_IMPLICIT_CASTS_H
#define CLANG_LAZY_IMPLICIT_CASTS_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang {
class ImplicitCastExpr;
class SourceLocation;
class Stmt;
class CallExpr;
class FunctionDecl;
}

/**
 * Finds places with unwanted implicit casts.
 *
 * See README-implicit-casts for more information
 */
class ImplicitCasts
    : public CheckBase
{
public:
    ImplicitCasts(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
private:
    bool isBoolToInt(clang::FunctionDecl *func) const;
    bool isMacroToIgnore(clang::SourceLocation loc) const;
};

#endif
