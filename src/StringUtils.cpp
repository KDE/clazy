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

#include "StringUtils.h"

#include <string>
#include <vector>

namespace clang {
class LangOptions;
}  // namespace clang

using namespace std;
using namespace clang;

std::string clazy::simpleArgTypeName(clang::FunctionDecl *func, unsigned int index, const clang::LangOptions &lo)
{
    if (!func || index >= func->getNumParams())
        return {};

    ParmVarDecl *parm = func->getParamDecl(index);
    return simpleTypeName(parm, lo);
}

bool clazy::anyArgIsOfSimpleType(clang::FunctionDecl *func,
                                 const std::string &simpleType,
                                 const clang::LangOptions &lo)
{
    if (!func)
        return false;

    return clazy::any_of(Utils::functionParameters(func), [simpleType, lo](ParmVarDecl *p){
        return simpleTypeName(p, lo) == simpleType;
    });
}

bool clazy::anyArgIsOfAnySimpleType(clang::FunctionDecl *func,
                                    const vector<string> &simpleTypes,
                                    const clang::LangOptions &lo)
{
    if (!func)
        return false;

    return clazy::any_of(simpleTypes, [func, lo](const string &simpleType) {
        return clazy::anyArgIsOfSimpleType(func, simpleType, lo);
    });
}
