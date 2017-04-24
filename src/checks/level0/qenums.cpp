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

#include "qenums.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Token.h>
#include <clang/Lex/Preprocessor.h>

using namespace clang;
using namespace std;

Qenums::Qenums(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
}

void Qenums::VisitMacroExpands(const Token &MacroNameTok, const SourceRange &range)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii || ii->getName() != "Q_ENUMS")
        return;

    if (range.getBegin().isMacroID())
        return;

    if (sm().isInSystemHeader(range.getBegin()))
        return;

    emitWarning(range.getBegin(), "Use Q_ENUM instead of Q_ENUMS");
}

REGISTER_CHECK_WITH_FLAGS("qenums", Qenums, CheckLevel0)
