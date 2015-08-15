/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "missingtypeinfo.h"
#include "Utils.h"

#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

#include <sstream>

using namespace std;
using namespace clang;

MissingTypeinfo::MissingTypeinfo(clang::CompilerInstance &ci)
    : CheckBase(ci)
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
            s = "Q_DECLARE_PRIMITIVE candidate: " + typeName;
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
    std::vector<std::string> primitives {"QPair"};
    return std::find(primitives.begin(), primitives.end(), className) != primitives.end();
}

std::string MissingTypeinfo::name() const
{
    return "missing-typeinfo";
}
