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

REGISTER_CHECK("nrvo", NRVOEnabler)
