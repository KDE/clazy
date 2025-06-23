/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ifndef-define-typo.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "levenshteindistance.h"

#include <clang/AST/AST.h>

using namespace clang;

IfndefDefineTypo::IfndefDefineTypo(const std::string &name)
    : CheckBase(name)
{
}

void IfndefDefineTypo::VisitMacroDefined(const Token &macroNameTok)
{
    if (!m_lastIfndef.empty()) {
        if (IdentifierInfo *ii = macroNameTok.getIdentifierInfo()) {
            maybeWarn(static_cast<std::string>(ii->getName()), macroNameTok.getLocation());
        }
    }
}

void IfndefDefineTypo::VisitDefined(const Token &macroNameTok, const SourceRange &)
{
    if (!m_lastIfndef.empty()) {
        if (IdentifierInfo *ii = macroNameTok.getIdentifierInfo()) {
            maybeWarn(static_cast<std::string>(ii->getName()), macroNameTok.getLocation());
        }
    }
}

void IfndefDefineTypo::VisitIfdef(SourceLocation, const Token &)
{
    m_lastIfndef.clear();
}

void IfndefDefineTypo::VisitIfndef(SourceLocation, const Token &macroNameTok)
{
    if (IdentifierInfo *ii = macroNameTok.getIdentifierInfo()) {
        m_lastIfndef = static_cast<std::string>(ii->getName());
    }
}

void IfndefDefineTypo::VisitIf(SourceLocation, SourceRange, PPCallbacks::ConditionValueKind)
{
    m_lastIfndef.clear();
}

void IfndefDefineTypo::VisitElif(SourceLocation, SourceRange, PPCallbacks::ConditionValueKind, SourceLocation)
{
    m_lastIfndef.clear();
}

void IfndefDefineTypo::VisitElse(SourceLocation, SourceLocation)
{
    m_lastIfndef.clear();
}

void IfndefDefineTypo::VisitEndif(SourceLocation, SourceLocation)
{
    m_lastIfndef.clear();
}

void IfndefDefineTypo::maybeWarn(const std::string &define, SourceLocation loc)
{
    if (m_lastIfndef == "Q_CONSTRUCTOR_FUNCTION") { // Transform into a list if more false-positives need to be added
        return;
    }

    if (define == m_lastIfndef) {
        m_lastIfndef.clear();
        return;
    }

    if (define.length() < 4) {
        return;
    }

    const int levDistance = levenshtein_distance(define, m_lastIfndef);
    if (levDistance < 3) {
        emitWarning(loc, std::string("Possible typo in define. ") + m_lastIfndef + " vs " + define);
    }
}
