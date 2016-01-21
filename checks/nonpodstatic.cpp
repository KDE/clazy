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

#include "nonpodstatic.h"
#include "Utils.h"
#include "StringUtils.h"
#include "checkmanager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

static bool shouldIgnoreType(const std::string &name, const clang::CompilerInstance &ci)
{
    // Q_GLOBAL_STATIC and such
    static vector<string> blacklist = {"Holder", "AFUNC", "QLoggingCategory"};
    return find(blacklist.cbegin(), blacklist.cend(), name) != blacklist.cend();
}

NonPodStatic::NonPodStatic(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void NonPodStatic::VisitStmt(clang::Stmt *stm)
{
    VarDecl *varDecl = m_lastDecl ? dyn_cast<VarDecl>(m_lastDecl) : nullptr;
    if (!varDecl || varDecl->isConstexpr() || varDecl->isExternallyVisible() || !varDecl->isFileVarDecl())
        return;

    StorageDuration sd = varDecl->getStorageDuration();
    if (sd != StorageDuration::SD_Static)
        return;

    const SourceLocation declStart = varDecl->getLocStart();
    auto macroName = Lexer::getImmediateMacroName(declStart, m_ci.getSourceManager(), m_ci.getLangOpts());
    if (stringStartsWith(macroName, "Q_CONSTRUCTOR_FUNCTION") ||
        stringStartsWith(macroName, "Q_DESTRUCTOR_FUNCTION")) // Don't warn on these
        return;

    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!ctorExpr)
        return;

    auto ctorDecl = ctorExpr->getConstructor();
    auto recordDecl = ctorDecl ? ctorDecl->getParent() : nullptr;
    if (!recordDecl)
        return;

    if (recordDecl->hasTrivialDestructor()) {
        // Has a trivial dtor, but now lets check the ctors.

        if (ctorDecl->isDefaultConstructor() && recordDecl->hasTrivialDefaultConstructor()) {
            // both dtor and called ctor are trivial, no warning
            return;
        } else if (ctorDecl->isConstexpr()) {
            // Used ctor is constexpr, it's fine
            return;
        }
    }

    const string className = recordDecl->getName();
    if (!shouldIgnoreType(className, m_ci)) {
        std::string error = "non-POD static (" + className + ')';
        emitWarning(declStart, error.c_str());
    }

}

std::vector<string> NonPodStatic::filesToIgnore() const
{
    return {"main.cpp", "qrc_", "qdbusxml2cpp"};
}

REGISTER_CHECK_WITH_FLAGS("non-pod-global-static", NonPodStatic, CheckLevel1)
