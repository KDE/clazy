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

#include "missingtypeinfo.h"
#include "Utils.h"
#include "TemplateUtils.h"
#include "TypeUtils.h"
#include "QtUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

#include <sstream>

using namespace std;
using namespace clang;

MissingTypeinfo::MissingTypeinfo(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void MissingTypeinfo::VisitDecl(clang::Decl *decl)
{
    // Catches QTypeInfo<Foo> to know type classification
    auto templateDef = dyn_cast<ClassTemplateSpecializationDecl>(decl);
    if (templateDef)
        registerQTypeInfo(templateDef);

    // Catches QList<Foo>
    ClassTemplateSpecializationDecl *tstdecl = TemplateUtils::templateDecl(decl);
    if (!tstdecl)
        return;

    const bool isQList = tstdecl->getName() == "QList";
    const bool isQVector = tstdecl->getName() == "QVector";

    if (!isQList && !isQVector)
        return;

    const TemplateArgumentList &tal = tstdecl->getTemplateArgs();

    if (tal.size() != 1) return;
    QualType qt2 = tal[0].getAsType();

    const Type *t = qt2.getTypePtrOrNull();
    CXXRecordDecl *record = t ? t->getAsCXXRecordDecl() : nullptr;
    if (!record || !record->getDefinition())
        return; // Don't crash if we only have a fwd decl

    const bool isCopyable = qt2.isTriviallyCopyableType(m_ci.getASTContext());
    const bool isTooBigForQList = QtUtils::isTooBigForQList(qt2, m_ci);

    if (isCopyable && (isQVector || (isQList && isTooBigForQList))) {
        if (m_typeInfos.count(record->getQualifiedNameAsString()) != 0)
            return;

        std::string typeName = record->getName();
        std::string s;
        std::stringstream out;
        out << m_ci.getASTContext().getTypeSize(qt2)/8;
        s = "Missing Q_DECLARE_TYPEINFO: " + typeName;
        emitWarning(decl->getLocStart(), s.c_str());
        emitWarning(record->getLocStart(), "Type declared here:", false);
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

bool MissingTypeinfo::ignoreTypeInfo(const std::string &className) const
{
    std::vector<std::string> primitives {"QPair", "QTime"}; // clazy bug
    return clazy_std::contains(primitives, className);
}


REGISTER_CHECK_WITH_FLAGS("missing-typeinfo", MissingTypeinfo, CheckLevel3)
