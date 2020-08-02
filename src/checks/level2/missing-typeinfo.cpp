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

#include "missing-typeinfo.h"
#include "TemplateUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>

class ClazyContext;
namespace clang {
class Decl;
}  // namespace clang

using namespace std;
using namespace clang;

MissingTypeInfo::MissingTypeInfo(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void MissingTypeInfo::VisitDecl(clang::Decl *decl)
{
    ClassTemplateSpecializationDecl *tstdecl = clazy::templateDecl(decl);
    if (!tstdecl)
        return;

    const bool isQList = clazy::name(tstdecl) == "QList";
    const bool isQVector = isQList ? false : clazy::name(tstdecl) == "QVector";

    if (!isQList && !isQVector) {
        registerQTypeInfo(tstdecl);
        return;
    }

    QualType qt2 = clazy::getTemplateArgumentType(tstdecl, 0);
    const Type *t = qt2.getTypePtrOrNull();
    CXXRecordDecl *record = t ? t->getAsCXXRecordDecl() : nullptr;
    if (!record || !record->getDefinition() || typeHasClassification(qt2))
        return; // Don't crash if we only have a fwd decl

    const bool isCopyable = qt2.isTriviallyCopyableType(m_astContext);
    const bool isTooBigForQList = isQList && clazy::isTooBigForQList(qt2, &m_astContext);

    if ((isQVector || isTooBigForQList) && isCopyable) {
        if (sm().isInSystemHeader(clazy::getLocStart(record)))
            return;

        std::string typeName = static_cast<std::string>(clazy::name(record));
        if (typeName == "QPair") // QPair doesn't use Q_DECLARE_TYPEINFO, but rather a explicit QTypeInfo.
            return;

        emitWarning(decl, "Missing Q_DECLARE_TYPEINFO: " + typeName);
        emitWarning(record, "Type declared here:", false);
    }
}

void MissingTypeInfo::registerQTypeInfo(ClassTemplateSpecializationDecl *decl)
{
    if (clazy::name(decl) == "QTypeInfo") {
        const string typeName = clazy::getTemplateArgumentTypeStr(decl, 0, lo(), /**recordOnly=*/ true);
        if (!typeName.empty())
            m_typeInfos.insert(typeName);
    }
}

bool MissingTypeInfo::typeHasClassification(QualType qt) const
{
    return m_typeInfos.find(clazy::simpleTypeName(qt, lo())) != m_typeInfos.end();
}
