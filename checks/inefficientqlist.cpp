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

#include "inefficientqlist.h"
#include "Utils.h"
#include "TemplateUtils.h"
#include "checkmanager.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

using namespace clang;
using namespace std;

enum IgnoreMode {
    None = 0,
    NonLocalVariable = 1,
    InFunctionWithSameReturnType = 2,
    IsAssignedTooInFunction = 4
};

InefficientQList::InefficientQList(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

static bool shouldIgnoreVariable(VarDecl *varDecl, int ignoreMode)
{
    DeclContext *context = varDecl->getDeclContext();
    FunctionDecl *fDecl = context ? dyn_cast<FunctionDecl>(context) : nullptr;

    if (ignoreMode & NonLocalVariable) {
        if (!fDecl)
            return true;
    }

    if (ignoreMode & IsAssignedTooInFunction) {
        if (fDecl && fDecl->getReturnType() == varDecl->getType())
            return true;
    }

    if (ignoreMode & IsAssignedTooInFunction) {
        if (fDecl) {
            if (Utils::containsAssignment(fDecl->getBody(), varDecl))
                return true;
        }
    }

    return false;
}

void InefficientQList::VisitDecl(clang::Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl)
        return;

    QualType type = varDecl->getType();
    const Type *t = type.getTypePtrOrNull();
    if (!t)
        return;

    if (shouldIgnoreVariable(varDecl, NonLocalVariable | InFunctionWithSameReturnType | IsAssignedTooInFunction))
        return;

    CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
    if (!recordDecl || recordDecl->getNameAsString() != "QList")
        return;

    const std::vector<clang::QualType> types = TemplateUtils::getTemplateArgumentsTypes(recordDecl);
    if (types.empty())
        return;
    QualType qt2 = types[0];
    const int size_of_void = 8 * 8; // 64 bits
    const int size_of_T = m_ci.getASTContext().getTypeSize(qt2);

    if (size_of_T > size_of_void) {
        string s = string("Use QVector instead of QList for type with size " + to_string(size_of_T / 8) + " bytes");
        emitWarning(decl->getLocStart(), s.c_str());
    }
}

REGISTER_CHECK_WITH_FLAGS("inefficient-qlist", InefficientQList, CheckLevel3)
