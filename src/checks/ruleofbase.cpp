/*
    This file is part of the clazy static checker.

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

#include "ruleofbase.h"
#include "StringUtils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>

#include <vector>

class ClazyContext;

using namespace clang;
using namespace std;

RuleOfBase::RuleOfBase(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

bool RuleOfBase::isBlacklisted(CXXRecordDecl *record) const
{
    if (!record || clazy::startsWith(record->getQualifiedNameAsString(), "std::"))
        return true;

    const auto qualifiedName = clazy::classNameFor(record);

    static const vector<string> blacklisted = { "QAtomicInt", "QBasicAtomicInteger", "QAtomicInteger", "QBasicAtomicPointer",
                                                "QList::iterator", "QList::const_iterator", "QTextBlock::iterator",
                                                "QAtomicPointer", "QtPrivate::ConverterMemberFunction",
                                                "QtPrivate::ConverterMemberFunctionOk", "QtPrivate::ConverterFunctor",
                                                "QtMetaTypePrivate::VariantData", "QScopedArrayPointer",
                                                "QtPrivate::AlignOfHelper", "QColor", "QCharRef", "QByteRef",
                                                "QObjectPrivate::Connection", "QMutableListIterator",
                                                "QStringList", "QVariant::Private",
                                                "QModelIndex", // Qt4
                                                "QPair", // Qt4
                                                "QSet", // Fixed for Qt 5.7
                                                "QSet::iterator",
                                                "QSet::const_iterator",
                                                "QLinkedList::iterator",
                                                "QLinkedList::const_iterator",
                                                "QJsonArray::const_iterator",
                                                "QJsonArray::iterator",
                                                "QTextFrame::iterator",
                                                "QFuture::const_iterator",
                                                "QFuture::iterator",
                                                "QMatrix",
                                                "QBitRef", "QJsonValueRef",
                                                "QTypedArrayData::iterator"
    };
    return clazy::contains(blacklisted, qualifiedName);
}
