/*
    This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "TemplateUtils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Decl.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

namespace clang {
class LangOptions;
}  // namespace clang

using namespace std;
using namespace clang;

static vector<QualType> typesFromTemplateArguments(const TemplateArgumentList *templateArgs)
{
    vector<QualType> result;
    const int numArgs = templateArgs->size();
    result.reserve(numArgs);
    for (int i = 0; i < numArgs; ++i) {
        const TemplateArgument &arg = templateArgs->get(i);
        if (arg.getKind() == TemplateArgument::Type)
            result.push_back(arg.getAsType());
    }

    return result;
}

vector<QualType> clazy::getTemplateArgumentsTypes(CXXMethodDecl *method)
{
    if (!method)
        return {};

    FunctionTemplateSpecializationInfo *specializationInfo = method->getTemplateSpecializationInfo();
    if (!specializationInfo || !specializationInfo->TemplateArguments)
        return {};

    return typesFromTemplateArguments(specializationInfo->TemplateArguments);
}

std::vector<clang::QualType> clazy::getTemplateArgumentsTypes(CXXRecordDecl *record)
{
    if (!record)
        return {};

    ClassTemplateSpecializationDecl *templateDecl = dyn_cast<ClassTemplateSpecializationDecl>(record);
    if (!templateDecl)
        return {};

    return typesFromTemplateArguments(&(templateDecl->getTemplateInstantiationArgs()));
}

ClassTemplateSpecializationDecl *clazy::templateDecl(Decl *decl)
{
    if (isa<ClassTemplateSpecializationDecl>(decl))
        return dyn_cast<ClassTemplateSpecializationDecl>(decl);

    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl) return nullptr;
    QualType qt = varDecl->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) return nullptr;
    CXXRecordDecl *classDecl = t->getAsCXXRecordDecl();
    if (!classDecl) return nullptr;
    return dyn_cast<ClassTemplateSpecializationDecl>(classDecl);
}

string clazy::getTemplateArgumentTypeStr(ClassTemplateSpecializationDecl *specialization,
                                         unsigned int index, const LangOptions &lo, bool recordOnly)
{
    if (!specialization)
        return {};

    auto &args = specialization->getTemplateArgs();
    if (args.size() <= index)
        return {};

    QualType qt = args[index].getAsType();
    if (recordOnly) {
        const Type *t = qt.getTypePtrOrNull();
        if (!t || !t->getAsCXXRecordDecl())
            return {};
    }

    return clazy::simpleTypeName(args[index].getAsType(), lo);
}

clang::QualType clazy::getTemplateArgumentType(ClassTemplateSpecializationDecl *specialization, unsigned int index)
{
    if (!specialization)
        return {};

    auto &args = specialization->getTemplateArgs();
    if (args.size() <= index)
        return {};

    return args[index].getAsType();
}
