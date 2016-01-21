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

void NonPodStatic::VisitDecl(clang::Decl *decl)
{
    auto varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl || varDecl->isConstexpr() || varDecl->isExternallyVisible() || !varDecl->isFileVarDecl())
        return;

    StorageDuration sd = varDecl->getStorageDuration();
    if (sd != StorageDuration::SD_Static)
        return;

    QualType qt = varDecl->getType();
    const bool isTrivial = qt.isTrivialType(m_ci.getASTContext());
    if (isTrivial)
        return;

    const Type *t = qt.getTypePtrOrNull();
    if (!t || !t->getAsCXXRecordDecl() || t->getAsCXXRecordDecl()->isLiteral())
        return;

    const SourceLocation declStart = decl->getLocStart();

    auto macroName = Lexer::getImmediateMacroName(declStart, m_ci.getSourceManager(), m_ci.getLangOpts());
    if (stringStartsWith(macroName, "Q_CONSTRUCTOR_FUNCTION")) // Don't warn on these
        return;

    const string className = t->getAsCXXRecordDecl()->getName();
    if (!shouldIgnoreType(className, m_ci)) {
        std::string error = "non-POD static (" + className + ')';
        emitWarning(declStart, error.c_str());
    }
}

std::vector<string> NonPodStatic::filesToIgnore() const
{
    return {"main.cpp", "qrc_"};
}

REGISTER_CHECK_WITH_FLAGS("non-pod-global-static", NonPodStatic, CheckLevel1)
