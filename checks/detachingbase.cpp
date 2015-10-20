/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
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

DetachingBase::DetachingBase(const std::string &name)
    : CheckBase(name)
{
    m_methodsByType["QList"] = {"first", "last", "begin", "end", "front", "back"};
    m_methodsByType["QVector"] = {"first", "last", "begin", "end", "front", "back", "data" };
    m_methodsByType["QMap"] = {"begin", "end", "first", "find", "last", "lowerBound", "upperBound" };
    m_methodsByType["QHash"] = {"begin", "end", "find" };
    m_methodsByType["QLinkedList"] = {"first", "last", "begin", "end", "front", "back" };
    m_methodsByType["QSet"] = {"begin", "end", "find" };
    m_methodsByType["QStack"] = {"top"};
    m_methodsByType["QQueue"] = {"head"};
    m_methodsByType["QMultiMap"] = m_methodsByType["QMap"];
    m_methodsByType["QMultiHash"] = m_methodsByType["QHash"];
    m_methodsByType["QString"] = {"begin", "end", "data", "operator[]", "push_back", "push_front", "clear", "chop"};
    m_methodsByType["QByteArray"] = {"data"};
    m_methodsByType["QImage"] = {"bits", "scanLine"};

    m_writeMethodsByType["QList"] = {"takeAt", "takeFirst", "takeLast", "removeOne", "removeAll", "erase", "operator[]"};
    m_writeMethodsByType["QVector"] = { "fill", "insert", "operator[]" };
    m_writeMethodsByType["QMap"] = { "erase", "insert", "insertMulti", "remove", "take", "unite", "operator[]" };
    m_writeMethodsByType["QHash"] = { "erase", "insert", "insertMulti", "remove", "take", "unite", "operator[]" };
    m_writeMethodsByType["QMultiHash"] = m_writeMethodsByType["QHash"];
    m_writeMethodsByType["QMultiMap"] = m_writeMethodsByType["QMap"];
    m_writeMethodsByType["QLinkedList"] = {"takeFirst", "takeLast", "removeOne", "removeAll", "erase", "operator[]"};
    m_writeMethodsByType["QSet"] = {"erase", "insert", "intersect", "unite", "subtract", "operator[]"};
    m_writeMethodsByType["QStack"] = {"push", "swap", "operator[]"};
    m_writeMethodsByType["QQueue"] = {"enqueue", "swap", "operator[]"};
    m_writeMethodsByType["QListSpecialMethods"] = {"sort", "replaceInStrings", "removeDuplicates"};
    m_writeMethodsByType["QStringList"] = m_writeMethodsByType["QListSpecialMethods"];
}
