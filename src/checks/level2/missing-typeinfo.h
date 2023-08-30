/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MISSING_TYPE_INFO_H
#define CLAZY_MISSING_TYPE_INFO_H

#include "checkbase.h"

#include <set>
#include <string>

class ClazyContext;

namespace clang
{
class ClassTemplateSpecializationDecl;
class CXXRecordDecl;
class Decl;
class QualType;
}

/**
 * Suggests usage of Q_PRIMITIVE_TYPE or Q_MOVABLE_TYPE in cases where you're using QList<T> and sizeof(T) > sizeof(void*)
 * or using QVector<T>. Unless they already have a classification.
 *
 * See README-missing-type-info for more info.
 */
class MissingTypeInfo : public CheckBase
{
public:
    MissingTypeInfo(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
    void registerQTypeInfo(clang::ClassTemplateSpecializationDecl *decl);
    bool typeHasClassification(clang::QualType) const;
    std::set<std::string> m_typeInfos;
};

#endif
