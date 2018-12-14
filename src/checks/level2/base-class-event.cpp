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

#include "base-class-event.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <array>
#include <vector>

class ClazyContext;
namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;

BaseClassEvent::BaseClassEvent(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void BaseClassEvent::VisitDecl(Decl *decl)
{
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !method->hasBody() || !method->isThisDeclarationADefinition())
        return;

    const string methodName = method->getNameAsString();
    const bool isEvent = methodName == "event";
    const bool isEventFilter = isEvent ? false : methodName == "eventFilter";

    if (!isEvent && !isEventFilter)
        return;

    CXXRecordDecl *classDecl = method->getParent();
    if (!clazy::isQObject(classDecl))
        return;

    const string className = classDecl->getQualifiedNameAsString();
    if (clazy::contains(std::array<StringRef, 2>({"QObject", "QWidget"}), className))
        return;

    CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(classDecl);
    const string baseClassName = baseClass ? baseClass->getQualifiedNameAsString()
                                           : string("BaseClass");

    if (isEventFilter && clazy::contains(std::array<StringRef, 2>({"QObject", "QWidget"}), baseClassName)) {
        // This is fine, QObject and QWidget eventFilter() don't do anything
        return;
    }

    Stmt *body = method->getBody();
    std::vector<ReturnStmt*> returns;
    clazy::getChilds<ReturnStmt>(body, /*by-ref*/ returns);
    for (ReturnStmt *returnStmt : returns) {
        Stmt *maybeBoolExpr = clazy::childAt(returnStmt, 0);
        if (!maybeBoolExpr)
            continue;
        auto boolExpr = dyn_cast<CXXBoolLiteralExpr>(maybeBoolExpr);
        if (!boolExpr || boolExpr->getValue()) // if getValue() is true that's a return true, which is fine
            continue;

        emitWarning(clazy::getLocStart(returnStmt), "Return " + baseClassName + "::" + methodName + "() instead of false");
    }
}
