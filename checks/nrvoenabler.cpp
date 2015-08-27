/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "nrvoenabler.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

NRVOEnabler::NRVOEnabler(const std::string &name)
    : CheckBase(name)
{

}

void NRVOEnabler::VisitDecl(clang::Decl *decl)
{
    FunctionDecl *fDecl = dyn_cast<FunctionDecl>(decl);
    if (fDecl == nullptr || !fDecl->hasBody())
        return;

    QualType qt = fDecl->getReturnType();
    const Type *t = qt.getTypePtrOrNull();
    if (t == nullptr || t->isDependentType() || t->isDependentSizedArrayType()) // Avoid some assert inside clang
        return;

    if (t->isReferenceType() || t->isPointerType())
        return;

    const int size_of_T = m_ci.getASTContext().getTypeSize(qt);
    if (size_of_T >= 512) {
        vector<ReturnStmt*> returns;
        Utils::getChilds2(fDecl->getBody(), returns);
        for (ReturnStmt *ret : returns) {
            vector<CXXConstructExpr*> cexprs;
            Utils::getChilds2(ret, cexprs);
            if (!cexprs.empty()) {
                //llvm::errs() << "isElidable=" << cexprs.at(0)->isElidable() << "\n";
            }
        }

        std::string error = "returned type is large (" + std::to_string(size_of_T) + " bytes )" + qt.getAsString();
        emitWarning(fDecl->getLocStart(), error.c_str());
    }
}

void NRVOEnabler::VisitStmt(clang::Stmt *)
{
}

// REGISTER_CHECK("nrvo", NRVOEnabler)
