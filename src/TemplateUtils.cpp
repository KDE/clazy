/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "TemplateUtils.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

namespace clang
{
class LangOptions;
} // namespace clang

using namespace clang;

static std::vector<QualType> typesFromTemplateArguments(const TemplateArgumentList *templateArgs)
{
    std::vector<QualType> result;
    const int numArgs = templateArgs->size();
    result.reserve(numArgs);
    for (int i = 0; i < numArgs; ++i) {
        const TemplateArgument &arg = templateArgs->get(i);
        if (arg.getKind() == TemplateArgument::Type) {
            result.push_back(arg.getAsType());
        }
    }

    return result;
}

std::vector<QualType> clazy::getTemplateArgumentsTypes(CXXMethodDecl *method)
{
    if (!method) {
        return {};
    }

    FunctionTemplateSpecializationInfo *specializationInfo = method->getTemplateSpecializationInfo();
    if (!specializationInfo || !specializationInfo->TemplateArguments) {
        return {};
    }

    return typesFromTemplateArguments(specializationInfo->TemplateArguments);
}

std::vector<clang::QualType> clazy::getTemplateArgumentsTypes(CXXRecordDecl *record)
{
    if (!record) {
        return {};
    }

    auto *templateDecl = dyn_cast<ClassTemplateSpecializationDecl>(record);
    if (!templateDecl) {
        return {};
    }

    return typesFromTemplateArguments(&(templateDecl->getTemplateInstantiationArgs()));
}

ClassTemplateSpecializationDecl *clazy::templateDecl(Decl *decl)
{
    if (isa<ClassTemplateSpecializationDecl>(decl)) {
        return dyn_cast<ClassTemplateSpecializationDecl>(decl);
    }

    auto *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl) {
        return nullptr;
    }
    QualType qt = varDecl->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) {
        return nullptr;
    }
    CXXRecordDecl *classDecl = t->getAsCXXRecordDecl();
    if (!classDecl) {
        return nullptr;
    }
    return dyn_cast<ClassTemplateSpecializationDecl>(classDecl);
}

std::string clazy::getTemplateArgumentTypeStr(ClassTemplateSpecializationDecl *specialization, unsigned int index, const LangOptions &lo, bool recordOnly)
{
    if (!specialization) {
        return {};
    }

    const auto &args = specialization->getTemplateArgs();
    if (args.size() <= index) {
        return {};
    }

    QualType qt = args[index].getAsType();
    if (recordOnly) {
        const Type *t = qt.getTypePtrOrNull();
        if (!t || !t->getAsCXXRecordDecl()) {
            return {};
        }
    }

    return clazy::simpleTypeName(args[index].getAsType(), lo);
}

clang::QualType clazy::getTemplateArgumentType(ClassTemplateSpecializationDecl *specialization, unsigned int index)
{
    if (!specialization) {
        return {};
    }

    const auto &args = specialization->getTemplateArgs();
    if (args.size() <= index) {
        return {};
    }

    return args[index].getAsType();
}
