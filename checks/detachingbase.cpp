/*
   This file is part of the clang-lazy static checker.

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

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingBase::DetachingBase(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
    m_methodsByType["QList"] = {"first", "last", "begin", "end", "front", "back", "operator[]"};
    m_methodsByType["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "operator[]" };
    m_methodsByType["QMap"] = {"begin", "end", "first", "find", "last", "lowerBound", "upperBound", "operator[]" };
    m_methodsByType["QHash"] = {"begin", "end", "find", "operator[]" };
    m_methodsByType["QLinkedList"] = {"first", "last", "begin", "end", "front", "back", "operator[]" };
    m_methodsByType["QSet"] = {"begin", "end", "find", "operator[]" };
    m_methodsByType["QStack"] = {"top"};
    m_methodsByType["QQueue"] = {"head"};
    m_methodsByType["QMultiMap"] = m_methodsByType["QMap"];
    m_methodsByType["QMultiHash"] = m_methodsByType["QHash"];
    m_methodsByType["QString"] = {"begin", "end", "data", "operator[]"};
    m_methodsByType["QByteArray"] = {"data", "operator[]"};
    m_methodsByType["QImage"] = {"bits", "scanLine"};
}

bool DetachingBase::isDetachingMethod(CXXMethodDecl *method) const
{
    if (!method)
        return false;

    CXXRecordDecl *record = method->getParent();
    if (!record)
        return false;

    const string className = record->getNameAsString();

    auto it = m_methodsByType.find(className);
    if (it != m_methodsByType.cend()) {
        const auto &methods = it->second;
        auto it2 = find(methods.cbegin(), methods.cend(), method->getNameAsString());
        if (it2 != methods.cend())
            return true;
    }

    return false;
}
