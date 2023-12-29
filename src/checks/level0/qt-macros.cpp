/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt-macros.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"
#include "clazy_stl.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

using namespace clang;

QtMacros::QtMacros(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

void QtMacros::VisitMacroDefined(const Token &MacroNameTok)
{
    if (m_OSMacroExists) {
        return;
    }

    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && clazy::startsWith(static_cast<std::string>(ii->getName()), "Q_OS_")) {
        m_OSMacroExists = true;
    }
}

void QtMacros::checkIfDef(const Token &macroNameTok, SourceLocation Loc)
{
    IdentifierInfo *ii = macroNameTok.getIdentifierInfo();
    if (!ii) {
        return;
    }

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
    if (!m_context->usingPreCompiledHeaders()) {
        checkIfDef(macroNameTok, range.getBegin());
    }
}

void QtMacros::VisitIfdef(SourceLocation loc, const Token &macroNameTok)
{
    if (!m_context->usingPreCompiledHeaders()) {
        checkIfDef(macroNameTok, loc);
    }
}
