/*
   This file is part of the clazy static checker.

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

#include <llvm/Config/llvm-config.h>

#if !defined(IS_OLD_CLANG)

#include "missing-qobject-macro.h"

#include "Utils.h"
#include "QtUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>

using namespace clang;
using namespace std;

MissingQ_OBJECT::MissingQ_OBJECT(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
    enablePreProcessorCallbacks();
}

void MissingQ_OBJECT::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && ii->getName() == "Q_OBJECT")
        registerQ_OBJECT(range.getBegin());
}

void MissingQ_OBJECT::VisitDecl(clang::Decl *decl)
{
    CXXRecordDecl *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !QtUtils::isQObject(record))
        return;

    if (record->getDescribedClassTemplate() != nullptr) // moc doesn't accept Q_OBJECT in templates
        return;

    if (m_checkManager->usingPreCompiledHeaders(m_ci))
        return;

    const SourceLocation startLoc = decl->getLocStart();

    for (const SourceLocation &loc : m_qobjectMacroLocations) {
        if (sm().getFileID(loc) != sm().getFileID(startLoc))
            continue; // Different file

        if (sm().isBeforeInSLocAddrSpace(startLoc, loc) && sm().isBeforeInSLocAddrSpace(loc, decl->getLocEnd()))
            return; // We found a Q_OBJECT after start and before end, it's ours.
    }

    emitWarning(startLoc, record->getQualifiedNameAsString() + " is missing a Q_OBJECT macro");
}

void MissingQ_OBJECT::registerQ_OBJECT(SourceLocation loc)
{
    m_qobjectMacroLocations.push_back(loc);
}

REGISTER_CHECK_WITH_FLAGS("missing-qobject-macro", MissingQ_OBJECT, CheckLevel1)

#endif // clang >= 3.7
