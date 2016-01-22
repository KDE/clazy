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

#include "missingtypeinfo.h"
#include "Utils.h"
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
    if (templateDef) {
        registerQTypeInfo(templateDef);
    }

    // Catches QList<Foo>
    ClassTemplateSpecializationDecl *tstdecl = Utils::templateDecl(decl);
    if (tstdecl == nullptr)
        return;

    const bool isQList = tstdecl->getName() == "QList";
    const bool isQVector = tstdecl->getName() == "QVector";

    if (tstdecl == nullptr || (!isQList && !isQVector))
        return;

    const TemplateArgumentList &tal = tstdecl->getTemplateArgs();

    if (tal.size() != 1) return;
    QualType qt2 = tal[0].getAsType();

    const Type *t = qt2.getTypePtrOrNull();
    if (t == nullptr || t->getAsCXXRecordDecl() == nullptr || t->getAsCXXRecordDecl()->getDefinition() == nullptr) return; // Don't crash if we only have a fwd decl

    const int size_of_void = 64; // TODO arm 32bit ?
    const int size_of_T = m_ci.getASTContext().getTypeSize(qt2);

    const bool isCopyable = qt2.isTriviallyCopyableType(m_ci.getASTContext());
    const bool isTooBigForQList = size_of_T <= size_of_void;

    if (isCopyable && (isQVector || (isQList && isTooBigForQList))) {

        std::string typeName = t->getAsCXXRecordDecl()->getName();
        if (m_typeInfos.count(t->getAsCXXRecordDecl()->getQualifiedNameAsString()) != 0)
            return;

        if (t->isRecordType() && !ignoreTypeInfo(typeName)) {
            std::string s;
            std::stringstream out;
            out << m_ci.getASTContext().getTypeSize(qt2)/8;
            s = "Missing Q_DECLARE_TYPEINFO: " + typeName;
            emitWarning(decl->getLocStart(), s.c_str());
            emitWarning(t->getAsCXXRecordDecl()->getLocStart(), "Type declared here:", false);
        }
    }
}

void MissingTypeinfo::registerQTypeInfo(ClassTemplateSpecializationDecl *decl)
{
    if (decl->getName() == "QTypeInfo") {
        auto &args = decl->getTemplateArgs();
        if (args.size() != 1)
            return;

        QualType qt = args[0].getAsType();
        const Type *t = qt.getTypePtrOrNull();
        CXXRecordDecl *recordDecl =  t ? t->getAsCXXRecordDecl() : nullptr;
        // llvm::errs() << qt.getAsString() << " foo\n";
        if (recordDecl != nullptr) {
            m_typeInfos.insert(recordDecl->getQualifiedNameAsString());
        }
    }
}

bool MissingTypeinfo::ignoreTypeInfo(const std::string &className) const
{
    std::vector<std::string> primitives {"QPair", "QTime"}; // clazy bug
    return std::find(primitives.begin(), primitives.end(), className) != primitives.end();
}


REGISTER_CHECK_WITH_FLAGS("missing-typeinfo", MissingTypeinfo, CheckLevel3)
