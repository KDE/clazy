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

#include "inefficientqlist.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

using namespace clang;
using namespace std;

InefficientQList::InefficientQList(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

void InefficientQList::VisitDecl(clang::Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (varDecl == nullptr)
        return;

    QualType type = varDecl->getType();
    const Type *t = type.getTypePtrOrNull();
    if (t == nullptr)
        return;

    DeclContext *context = varDecl->getDeclContext();
    FunctionDecl *fDecl = context ? dyn_cast<FunctionDecl>(context) : nullptr;
    if (context == nullptr || fDecl == nullptr) // only locals, feel free to change
        return;
    QualType returnType = fDecl->getReturnType();
    if (returnType == type)
        return;

    CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
    if (recordDecl == nullptr || recordDecl->getNameAsString() != "QList")
        return;

    ClassTemplateSpecializationDecl *tstdecl = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl);
    if (tstdecl == nullptr)
        return;

    const TemplateArgumentList &tal = tstdecl->getTemplateArgs();

    if (tal.size() != 1) return;
    QualType qt2 = tal[0].getAsType();
    const int size_of_void = 64; // performance on arm is more important
    const int size_of_T = m_ci.getASTContext().getTypeSize(qt2);

    if (size_of_T > size_of_void) {
        string s = string("Use QVector instead of QList for type with size [-Wmore-warnings-qlist]") + to_string(size_of_T);
        Utils::emitWarning(m_ci, decl->getLocStart(), s.c_str());
    }
}

std::string InefficientQList::name() const
{
    return "inefficient-qlist";
}
