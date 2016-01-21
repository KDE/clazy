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

#include "rangeloop.h"
#include "Utils.h"
#include "QtUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

RangeLoop::RangeLoop(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void RangeLoop::VisitStmt(clang::Stmt *stmt)
{
    if (auto rangeLoop = dyn_cast<CXXForRangeStmt>(stmt)) {
        processForRangeLoop(rangeLoop);
    }
}

void RangeLoop::processForRangeLoop(CXXForRangeStmt *rangeLoop)
{
    Expr *containerExpr = rangeLoop->getRangeInit();
    if (!containerExpr)
        return;

    QualType qt = containerExpr->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t || !t->isRecordType())
        return;

    checkPassByConstRefCorrectness(rangeLoop);

    if (qt.isConstQualified()) // const won't detach
        return;

    CXXRecordDecl *record = t->getAsCXXRecordDecl();
    if (QtUtils::isQtIterableClass(Utils::rootBaseClass(record))) {
        emitWarning(rangeLoop->getLocStart(), "c++11 range-loop might detach Qt container (" + record->getQualifiedNameAsString() + ')');
    }
}

void RangeLoop::checkPassByConstRefCorrectness(CXXForRangeStmt *rangeLoop)
{
    Utils::QualTypeClassification classif;
    auto varDecl = rangeLoop->getLoopVariable();
    bool success = Utils::classifyQualType(m_ci, varDecl, /*by-ref*/classif, rangeLoop);
    if (!success)
        return;

    if (classif.passBigTypeByConstRef || classif.passNonTriviallyCopyableByConstRef) {
        string error;
        const string paramStr = varDecl->getType().getAsString();
        if (classif.passBigTypeByConstRef) {
            error = "Missing reference in foreach with sizeof(T) = ";
            error += std::to_string(classif.size_of_T) + " bytes (" + paramStr + ')';
        } else if (classif.passNonTriviallyCopyableByConstRef) {
            error = "Missing reference in foreach with non trivial type (" + paramStr + ')';
        }

        // We ignore classif.passSmallTrivialByValue because it doesn't matter, the compiler is able
        // to optimize it, generating the same assembly, regardless of pass by value.
        emitWarning(varDecl->getLocStart(), error.c_str());
    }
}

REGISTER_CHECK_WITH_FLAGS("range-loop", RangeLoop, CheckLevel1)
