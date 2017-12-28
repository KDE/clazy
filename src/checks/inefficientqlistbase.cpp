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

#include "inefficientqlistbase.h"
#include "Utils.h"
#include "TypeUtils.h"
#include "ContextUtils.h"
#include "HierarchyUtils.h"
#include "TemplateUtils.h"
#include "StmtBodyRange.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

using namespace clang;
using namespace std;

InefficientQListBase::InefficientQListBase(const std::string &name, ClazyContext *context, int ignoreMode)
    : CheckBase(name, context)
    , m_ignoreMode(ignoreMode)
{
}

bool InefficientQListBase::shouldIgnoreVariable(VarDecl *varDecl) const
{
    DeclContext *context = varDecl->getDeclContext();
    FunctionDecl *fDecl = context ? dyn_cast<FunctionDecl>(context) : nullptr;

    if ((m_ignoreMode & IgnoreNonLocalVariable) && !clazy::isValueDeclInFunctionContext(varDecl)) {
        return true;
    }

    if ((m_ignoreMode & IgnoreInFunctionWithSameReturnType) && fDecl && fDecl->getReturnType().getCanonicalType() == varDecl->getType().getCanonicalType()) {
        return true;
    }

    Stmt *body = fDecl ? fDecl->getBody() : nullptr;
    if ((m_ignoreMode & IgnoreIsAssignedToInFunction) && Utils::isAssignedTo(body, varDecl)) {
        return true;
    }

    if ((m_ignoreMode & IgnoreIsPassedToFunctions) && Utils::isPassedToFunction(StmtBodyRange(body), varDecl, /*by-ref=*/ false)) {
        return true;
    }

    if ((m_ignoreMode & IgnoreIsInitializedByFunctionCall) && Utils::isInitializedExternally(varDecl)) {
        return true;
    }

    return false;
}

void InefficientQListBase::VisitDecl(clang::Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl)
        return;

    QualType type = varDecl->getType();
    const Type *t = type.getTypePtrOrNull();
    if (!t)
        return;

    CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
    if (!recordDecl || recordDecl->getName() != "QList")
        return;

    const std::vector<clang::QualType> types = clazy::getTemplateArgumentsTypes(recordDecl);
    if (types.empty())
        return;
    QualType qt2 = types[0];
    if (!qt2.getTypePtrOrNull())
        return;

    const int size_of_ptr = TypeUtils::sizeOfPointer(&m_astContext, qt2); // in bits
    const int size_of_T = m_astContext.getTypeSize(qt2);

    if (size_of_T > size_of_ptr && !shouldIgnoreVariable(varDecl)) {
        string s = string("Use QVector instead of QList for type with size " + to_string(size_of_T / 8) + " bytes");
        emitWarning(decl->getLocStart(), s.c_str());
    }
}
