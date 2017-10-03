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

#include "nonpodstatic.h"
#include "Utils.h"
#include "StringUtils.h"
#include "MacroUtils.h"
#include "QtUtils.h"
#include "checkmanager.h"
#include "ClazyContext.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

static bool shouldIgnoreType(const std::string &name)
{
    // Q_GLOBAL_STATIC and such
    static vector<string> blacklist = {"Holder", "AFUNC", "QLoggingCategory", "QThreadStorage"};
    return clazy_std::contains(blacklist, name);
}

NonPodStatic::NonPodStatic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    m_filesToIgnore = { "main.cpp", "qrc_", "qdbusxml2cpp" };
}

void NonPodStatic::VisitStmt(clang::Stmt *stm)
{
    VarDecl *varDecl = m_lastDecl ? dyn_cast<VarDecl>(m_lastDecl) : nullptr;
    if (!varDecl || varDecl->isConstexpr() || varDecl->isExternallyVisible() || !varDecl->isFileVarDecl())
        return;

    if (shouldIgnoreFile(stm->getLocStart()))
        return;

    StorageDuration sd = varDecl->getStorageDuration();
    if (sd != StorageDuration::SD_Static)
        return;

    const SourceLocation declStart = varDecl->getLocStart();
    auto macroName = Lexer::getImmediateMacroName(declStart, sm(), lo());
    if (clazy_std::startsWithAny(macroName, { "Q_IMPORT_PLUGIN", "Q_CONSTRUCTOR_FUNCTION", "Q_DESTRUCTOR_FUNCTION"})) // Don't warn on these
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

    if (m_context->isQtDeveloper() && QtUtils::isBootstrapping(m_preprocessorOpts))
        return;

    const string className = recordDecl->getName();
    if (!shouldIgnoreType(className)) {
        std::string error = "non-POD static (" + className + ')';
        emitWarning(declStart, error.c_str());
    }

}

REGISTER_CHECK("non-pod-global-static", NonPodStatic, CheckLevel1)
