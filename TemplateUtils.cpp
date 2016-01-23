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
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>

using namespace std;
using namespace clang;

static vector<QualType> typesFromTemplateArguments(const TemplateArgumentList &templateArgs)
{
    vector<QualType> result;
    const int numArgs = templateArgs.size();
    result.reserve(numArgs);
    for (int i = 0; i < numArgs; ++i) {
        const TemplateArgument &arg = templateArgs.get(i);
        result.push_back(arg.getAsType());
    }

    return result;
}

vector<QualType> TemplateUtils::getTemplateArgumentsTypes(CXXMethodDecl *method)
{
    if (!method)
        return {};

    FunctionTemplateSpecializationInfo *specializationInfo = method->getTemplateSpecializationInfo();
    if (!specializationInfo || !specializationInfo->TemplateArguments)
        return {};

    return typesFromTemplateArguments(*(specializationInfo->TemplateArguments));
}

std::vector<clang::QualType> TemplateUtils::getTemplateArgumentsTypes(CXXRecordDecl *record)
{
    if (!record)
        return {};

    ClassTemplateSpecializationDecl *templateDecl = dyn_cast<ClassTemplateSpecializationDecl>(record);
    if (!templateDecl)
        return {};

    return typesFromTemplateArguments(templateDecl->getTemplateInstantiationArgs());
}
