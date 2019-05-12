/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "static-pmf.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;
namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;


StaticPmf::StaticPmf(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void StaticPmf::VisitDecl(clang::Decl *decl)
{
    auto vardecl = dyn_cast<VarDecl>(decl);
    if (!vardecl || !vardecl->isStaticLocal())
        return;

    const Type *t = clazy::unpealAuto(vardecl->getType());
    if (!t)
        return;

    auto memberPointerType = dyn_cast<clang::MemberPointerType>(t);
    if (!memberPointerType || !memberPointerType->isMemberFunctionPointer())
        return;

    auto record = memberPointerType->getMostRecentCXXRecordDecl();
    if (!clazy::isQObject(record))
        return;

    emitWarning(vardecl, "Static pointer to member has portability issues");
}
