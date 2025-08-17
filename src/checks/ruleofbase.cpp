/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ruleofbase.h"
#include "StringUtils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>

#include <vector>

using namespace clang;

RuleOfBase::RuleOfBase(const std::string &name, Options options)
    : CheckBase(name, options)
{
}

bool RuleOfBase::isBlacklisted(CXXRecordDecl *record) const
{
    if (!record || clazy::startsWith(record->getQualifiedNameAsString(), "std::")) {
        return true;
    }

    const auto qualifiedName = clazy::classNameFor(record);

    static const std::vector<std::string> blacklisted = {
        "QAtomicInt",
        "QBasicAtomicInteger",
        "QAtomicInteger",
        "QBasicAtomicPointer",
        "QList::iterator",
        "QList::const_iterator",
        "QTextBlock::iterator",
        "QAtomicPointer",
        "QtPrivate::ConverterMemberFunction",
        "QtPrivate::ConverterMemberFunctionOk",
        "QtPrivate::ConverterFunctor",
        "QtMetaTypePrivate::VariantData",
        "QScopedArrayPointer",
        "QtPrivate::AlignOfHelper",
        "QColor",
        "QCharRef",
        "QByteRef",
        "QObjectPrivate::Connection",
        "QMutableListIterator",
        "QStringList",
        "QVariant::Private",
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
        "QBitRef",
        "QJsonValueRef",
        "QTypedArrayData::iterator",
    };
    return clazy::contains(blacklisted, qualifiedName);
}
