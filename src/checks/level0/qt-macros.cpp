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

#include "qt-macros.h"
#include "ClazyContext.h"
#include "clazy_stl.h"
#include "PreProcessorVisitor.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

using namespace clang;
using namespace std;

QtMacros::QtMacros(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

void QtMacros::VisitMacroDefined(const Token &MacroNameTok)
{
    if (m_OSMacroExists)
        return;

    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && clazy::startsWith(static_cast<std::string>(ii->getName()), "Q_OS_"))
        m_OSMacroExists = true;
}

void QtMacros::checkIfDef(const Token &macroNameTok, SourceLocation Loc)
{
    IdentifierInfo *ii = macroNameTok.getIdentifierInfo();
    if (!ii)
        return;

    PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
    if (preProcessorVisitor && preProcessorVisitor->qtVersion() < 51204 && ii->getName() == "Q_OS_WINDOWS") {
        // Q_OS_WINDOWS was introduced in 5.12.4
        emitWarning(Loc, "Q_OS_WINDOWS was only introduced in Qt 5.12.4, use Q_OS_WIN instead");
    } else if (!m_OSMacroExists && clazy::startsWith(static_cast<std::string>(ii->getName()), "Q_OS_")) {
        emitWarning(Loc, "Include qglobal.h before testing Q_OS_ macros");
    }
}

void QtMacros::VisitDefined(const Token &macroNameTok, const SourceRange &range)
{
    if (!m_context->usingPreCompiledHeaders())
        checkIfDef(macroNameTok, range.getBegin());
}

void QtMacros::VisitIfdef(SourceLocation loc, const Token &macroNameTok)
{
    if (!m_context->usingPreCompiledHeaders())
        checkIfDef(macroNameTok, loc);
}
