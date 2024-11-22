/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "static-pmf.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

StaticPmf::StaticPmf(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void StaticPmf::VisitDecl(clang::Decl *decl)
{
    auto *vardecl = dyn_cast<VarDecl>(decl);
    if (!vardecl || !vardecl->isStaticLocal()) {
        return;
    }

    const Type *t = clazy::unpealAuto(vardecl->getType());
    if (!t) {
        return;
    }

    const auto *memberPointerType = dyn_cast<clang::MemberPointerType>(t);
    if (!memberPointerType || !memberPointerType->isMemberFunctionPointer()) {
        return;
    }

    auto *record = memberPointerType->getMostRecentCXXRecordDecl();
    if (!clazy::isQObject(record)) {
        return;
    }

    emitWarning(vardecl, "Static pointer to member has portability issues");
}
