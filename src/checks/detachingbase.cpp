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

#include "checkmanager.h"
#include "detachingbase.h"
#include "Utils.h"
#include "StringUtils.h"
#include "QtUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingBase::DetachingBase(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

bool DetachingBase::isDetachingMethod(CXXMethodDecl *method) const
{
    if (!method)
        return false;

    CXXRecordDecl *record = method->getParent();
    if (!record)
        return false;

    const string className = record->getNameAsString();

    const std::unordered_map<string, std::vector<string> > &methodsByType = QtUtils::detachingMethods();
    auto it = methodsByType.find(className);
    if (it != methodsByType.cend()) {
        const auto &methods = it->second;
        if (clazy_std::contains(methods, method->getNameAsString()))
            return true;
    }

    return false;
}
