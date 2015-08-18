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

#include "qlistint.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

// TODO: Check if we're not in a method returning QList<int>


ListInt::ListInt(const std::string &name)
    : CheckBase(name)
{

}

void ListInt::VisitDecl(Decl *decl)
{
    auto templateSpecialization = Utils::templateSpecializationFromVarDecl(decl);
    if (templateSpecialization == nullptr || templateSpecialization->getName() != "QList")
        return;

    const TemplateArgumentList &tal = templateSpecialization->getTemplateArgs();
    if (tal.size() != 1)
        return;

    QualType qt = tal[0].getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (t == nullptr)
        return;

    if (t->isIntegerType())
        emitWarning(decl->getLocStart(), "Use QVarLengthArray instead of QList<int>");
}

void ListInt::VisitStmt(Stmt *)
{
}

// REGISTER_CHECK("qlist-of-int", ListInt)
