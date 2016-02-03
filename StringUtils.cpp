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
#include <sstream>
#include <vector>

using namespace std;
using namespace clang;

vector<string> StringUtils::splitString(const string &str, char separator)
{
    string token;
    vector<string> result;
    istringstream istream(str);
    while (std::getline(istream, token, separator)) {
        result.push_back(token);
    }

    return result;
}

vector<string> StringUtils::splitString(const char *str, char separator)
{
    if (!str)
        return {};

    return splitString(string(str), separator);
}


std::string StringUtils::simpleArgTypeName(clang::FunctionDecl *func, uint index, clang::LangOptions lo)
{
    if (!func || index >= func->getNumParams())
        return {};

    ParmVarDecl *parm = func->getParamDecl(index);
    if (!parm)
        return {};

    return simpleTypeName(parm->getType(), lo);
}
