/*
   This file is part of the clang-lazy static checker.

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

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6

#include "missing-qobject.h"

#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>
#include <clang/Parse/Parser.h>

using namespace clang;
using namespace std;

class PreprocessorCallbacks : public clang::PPCallbacks
{
public:
    PreprocessorCallbacks(MissingQ_OBJECT *q)
        : clang::PPCallbacks()
        , q(q)
    {
    }

    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD, SourceRange range, const MacroArgs*) override
    {
        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (ii && ii->getName() == "Q_OBJECT")
            q->registerQ_OBJECT(range.getBegin());
    }

    MissingQ_OBJECT *const q;
};

MissingQ_OBJECT::MissingQ_OBJECT(const std::string &name)
    : CheckBase(name)
    , m_preprocessorCallbacks(new PreprocessorCallbacks(this))
{
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
}

void MissingQ_OBJECT::VisitDecl(clang::Decl *decl)
{
    CXXRecordDecl *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !Utils::isQObject(record))
        return;

    if (record->getDescribedClassTemplate() != nullptr) // moc doesn't accept Q_OBJECT in templates
        return;

    const SourceLocation startLoc = decl->getLocStart();

    for (auto it = m_qobjectMacroLocations.cbegin(), end = m_qobjectMacroLocations.cend(); it != end; ++it) {
        const SourceLocation &loc = *it;
        if (m_ci.getSourceManager().getFileID(loc) != m_ci.getSourceManager().getFileID(startLoc))
            continue; // Different file

        if (m_ci.getSourceManager().isBeforeInSLocAddrSpace(startLoc, loc) && m_ci.getSourceManager().isBeforeInSLocAddrSpace(loc, decl->getLocEnd()))
            return; // We found a Q_OBJECT after start and before end, it's ours.
    }

    emitWarning(startLoc, record->getQualifiedNameAsString() + " is missing a Q_OBJECT macro");
}

void MissingQ_OBJECT::registerQ_OBJECT(SourceLocation loc)
{
    m_qobjectMacroLocations.push_back(loc);
}

REGISTER_CHECK_WITH_FLAGS("missing-qobject", MissingQ_OBJECT, CheckLevel1)

#endif // clang >= 3.7
