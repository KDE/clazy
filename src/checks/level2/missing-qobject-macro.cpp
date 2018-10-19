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

#include "missing-qobject-macro.h"
#include "ClazyContext.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

namespace clang {
class MacroInfo;
}  // namespace clang

using namespace clang;
using namespace std;

MissingQObjectMacro::MissingQObjectMacro(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
}

void MissingQObjectMacro::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && ii->getName() == "Q_OBJECT")
        registerQ_OBJECT(range.getBegin());
}

void MissingQObjectMacro::VisitDecl(clang::Decl *decl)
{
    CXXRecordDecl *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !clazy::isQObject(record))
        return;

    if (record->getDescribedClassTemplate() != nullptr) // moc doesn't accept Q_OBJECT in templates
        return;

    if (m_context->usingPreCompiledHeaders())
        return;

    const SourceLocation startLoc = getLocStart(decl);

    for (const SourceLocation &loc : m_qobjectMacroLocations) {
        if (sm().getFileID(loc) != sm().getFileID(startLoc))
            continue; // Different file

        if (sm().isBeforeInSLocAddrSpace(startLoc, loc) && sm().isBeforeInSLocAddrSpace(loc, getLocEnd(decl)))
            return; // We found a Q_OBJECT after start and before end, it's ours.
    }

    emitWarning(startLoc, record->getQualifiedNameAsString() + " is missing a Q_OBJECT macro");
}

void MissingQObjectMacro::registerQ_OBJECT(SourceLocation loc)
{
    m_qobjectMacroLocations.push_back(loc);
}
