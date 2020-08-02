/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "ifndef-define-typo.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "levenshteindistance.h"

#include <clang/AST/AST.h>

#include <iostream>

using namespace clang;
using namespace std;


IfndefDefineTypo::IfndefDefineTypo(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
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
    if (IdentifierInfo *ii = macroNameTok.getIdentifierInfo())
        m_lastIfndef = static_cast<std::string>(ii->getName());
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

void IfndefDefineTypo::maybeWarn(const string &define, SourceLocation loc)
{
    if (m_lastIfndef == "Q_CONSTRUCTOR_FUNCTION") // Transform into a list if more false-positives need to be added
        return;

    if (define == m_lastIfndef) {
        m_lastIfndef.clear();
        return;
    }

    if (define.length() < 4)
        return;

    const int levDistance = levenshtein_distance(define, m_lastIfndef);
    if (levDistance < 3) {
        emitWarning(loc, string("Possible typo in define. ") + m_lastIfndef + " vs " + define);
    }
}
