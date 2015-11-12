/*
   This file is part of the clang-lazy static checker.

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

#include "ruleofbase.h"
#include "StringUtils.h"

using namespace clang;
using namespace std;

RuleOfBase::RuleOfBase(const std::string &name)
    : CheckBase(name)
{
}

bool RuleOfBase::isBlacklisted(CXXRecordDecl *record) const
{
    if (!record)
        return true;

    auto qualifiedName = record->getQualifiedNameAsString();
    if (record->getNameAsString() == "iterator" || record->getNameAsString() == "const_iterator") {
        if (stringStartsWith(qualifiedName, "QList<")) // Fixed in Qt6
            return true;
    }

    static const vector<string> blacklisted = { "QAtomicInt", "QBasicAtomicInteger", "QAtomicInteger", "QBasicAtomicPointer",
                                                "QList::iterator", "QList::const_iterator", "QTextBlock::iterator",
                                                "QAtomicPointer", "QtPrivate::ConverterMemberFunction",
                                                "QtPrivate::ConverterMemberFunctionOk", "QtPrivate::ConverterFunctor",
                                                "QtMetaTypePrivate::VariantData", "QScopedArrayPointer",
                                                "QtPrivate::AlignOfHelper", "QColor", "QCharRef", "QByteRef",
                                                "QObjectPrivate::Connection", "QMutableListIterator", "QStringList"};
    return find(blacklisted.cbegin(), blacklisted.cend(), qualifiedName) != blacklisted.cend();
}
