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

#include "missing-type-info.h"
#include "Utils.h"
#include "TemplateUtils.h"
#include "TypeUtils.h"
#include "QtUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

using namespace std;
using namespace clang;

MissingTypeinfo::MissingTypeinfo(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void MissingTypeinfo::VisitDecl(clang::Decl *decl)
{
    ClassTemplateSpecializationDecl *tstdecl = TemplateUtils::templateDecl(decl);
    if (!tstdecl)
        return;

    const bool isQList = tstdecl->getName() == "QList";
    const bool isQVector = isQList ? false : tstdecl->getName() == "QVector";

    if (!isQList && !isQVector) {
        registerQTypeInfo(tstdecl);
        return;
    }

    QualType qt2 = TemplateUtils::getTemplateArgumentType(tstdecl, 0);
    const Type *t = qt2.getTypePtrOrNull();
    CXXRecordDecl *record = t ? t->getAsCXXRecordDecl() : nullptr;
    if (!record || !record->getDefinition() || typeHasClassification(qt2))
        return; // Don't crash if we only have a fwd decl

    const bool isCopyable = qt2.isTriviallyCopyableType(m_astContext);
    const bool isTooBigForQList = isQList && QtUtils::isTooBigForQList(qt2, &m_astContext);

    if ((isQVector || isTooBigForQList) && isCopyable) {
        if (sm().isInSystemHeader(record->getLocStart()))
            return;

        std::string typeName = record->getName();
        if (typeName == "QPair") // QPair doesn't use Q_DECLARE_TYPEINFO, but rather a explicit QTypeInfo.
            return;

        emitWarning(decl, "Missing Q_DECLARE_TYPEINFO: " + typeName);
        emitWarning(record, "Type declared here:", false);
    }
}

void MissingTypeinfo::registerQTypeInfo(ClassTemplateSpecializationDecl *decl)
{
    if (decl->getName() == "QTypeInfo") {
        const string typeName = TemplateUtils::getTemplateArgumentTypeStr(decl, 0, lo(), /**recordOnly=*/true);
        if (!typeName.empty())
            m_typeInfos.insert(typeName);
    }
}

bool MissingTypeinfo::typeHasClassification(QualType qt) const
{
    return m_typeInfos.find(StringUtils::simpleTypeName(qt, lo())) != m_typeInfos.end();
}

REGISTER_CHECK("missing-typeinfo", MissingTypeinfo, CheckLevel2)
