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

#include "empty-qstringliteral.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "PreProcessorVisitor.h"
#include "ClazyContext.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;


EmptyQStringliteral::EmptyQStringliteral(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void EmptyQStringliteral::VisitStmt(clang::Stmt *stmt)
{
    auto declstm = dyn_cast<DeclStmt>(stmt);
    if (!declstm || !declstm->isSingleDecl())
        return;

    auto vd = dyn_cast<VarDecl>(declstm->getSingleDecl());
    if (!vd || clazy::name(vd) != "qstring_literal")
        return;

    Expr *expr = vd->getInit();
    auto initListExpr = expr ? dyn_cast<InitListExpr>(expr) : nullptr;
    if (!initListExpr || initListExpr->getNumInits() != 2)
        return;

    Expr *init = initListExpr->getInit(1);
    auto literal = init ? dyn_cast<StringLiteral>(init) : nullptr;
    if (!literal || literal->getByteLength() != 0)
        return;

    if (!clazy::getLocStart(stmt).isMacroID())
        return;

    if (maybeIgnoreUic(clazy::getLocStart(stmt)))
        return;

    emitWarning(stmt, "Use an empty QLatin1String instead of an empty QStringLiteral");
}

bool EmptyQStringliteral::maybeIgnoreUic(SourceLocation loc) const
{
    PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;

    // Since 5.12 uic no longer uses QStringLiteral("")
    if (preProcessorVisitor && preProcessorVisitor->qtVersion() >= 51200)
        return false;

    return clazy::isUIFile(loc, sm());
}
