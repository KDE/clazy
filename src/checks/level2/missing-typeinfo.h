/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015-2016 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_MISSING_TYPE_INFO_H
#define CLAZY_MISSING_TYPE_INFO_H

#include "checkbase.h"

#include <set>
#include <string>

class ClazyContext;

namespace clang {
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
class MissingTypeInfo
    : public CheckBase
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
