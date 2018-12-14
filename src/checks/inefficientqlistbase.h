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

#ifndef INEFFICIENT_QLIST_BASE_H
#define INEFFICIENT_QLIST_BASE_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang {
class VarDecl;
class Decl;
}

/**
 * Base class for inefficient-qlist and inefficient-qlist-soft
 */
class InefficientQListBase
    : public CheckBase
{
public:
    enum IgnoreMode {
        IgnoreNone = 0,
        IgnoreNonLocalVariable = 1,
        IgnoreInFunctionWithSameReturnType = 2,
        IgnoreIsAssignedToInFunction = 4,
        IgnoreIsPassedToFunctions = 8,
        IgnoreIsInitializedByFunctionCall = 16
    };

    explicit InefficientQListBase(const std::string &name, ClazyContext *context, int ignoreMode);
    void VisitDecl(clang::Decl *decl) override;

private:
    bool shouldIgnoreVariable(clang::VarDecl *varDecl) const;
    const int m_ignoreMode;
};

#endif
