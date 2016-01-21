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

#include "checkbase.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMap.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>

#include <vector>

using namespace clang;
using namespace std;

CheckBase::CheckBase(const string &name, const CompilerInstance &ci)
    : m_ci(ci)
    , m_name(name)
    , m_lastDecl(nullptr)
    , m_enabledFixits(0)
{
    ASTContext &context = m_ci.getASTContext();
    m_tu = context.getTranslationUnitDecl();
    m_lastMethodDecl = nullptr;
}

CheckBase::~CheckBase()
{
}

void CheckBase::VisitStatement(Stmt *stm)
{
    if (!shouldIgnoreFile(stm->getLocStart())) {
        VisitStmt(stm);
    }
}

void CheckBase::VisitDeclaration(Decl *decl)
{
    if (shouldIgnoreFile(decl->getLocStart()))
        return;

    m_lastDecl = decl;
    auto mdecl = dyn_cast<CXXMethodDecl>(decl);
    if (mdecl)
        m_lastMethodDecl = mdecl;

    VisitDecl(decl);
}

string CheckBase::name() const
{
    return m_name;
}

void CheckBase::setParentMap(ParentMap *parentMap)
{
    m_parentMap = parentMap;
}

void CheckBase::VisitStmt(Stmt *)
{
}

void CheckBase::VisitDecl(Decl *)
{
}

bool CheckBase::shouldIgnoreFile(SourceLocation loc) const
{
    if (!loc.isValid() || m_ci.getSourceManager().isInSystemHeader(loc))
        return true;

    auto filename = m_ci.getSourceManager().getFilename(loc);

    const std::vector<std::string> files = filesToIgnore();
    for (auto &file : files) {
        bool contains = filename.find(file) != std::string::npos;
        if (contains)
            return true;
    }

    return false;
}

std::vector<std::string> CheckBase::filesToIgnore() const
{
    return {};
}

void CheckBase::emitWarning(clang::SourceLocation loc, std::string error, bool printWarningTag)
{
    emitWarning(loc, error, {}, printWarningTag);
}

void CheckBase::emitWarning(clang::SourceLocation loc, std::string error, const vector<FixItHint> &fixits, bool printWarningTag)
{
    if (loc.isMacroID()) {
        if (warningAlreadyEmitted(loc))
            return; // For warnings in macro arguments we get a warning in each place the argument is used within the expanded macro, so filter all the dups
        m_emittedWarningsInMacro.push_back(loc.getRawEncoding());
    }

    const string tag = " [-Wclazy-" + name() + ']';
    if (printWarningTag)
        error += tag;

    reallyEmitWarning(loc, error, fixits);

    for (auto l : m_queuedManualInterventionWarnings) {
        string msg = string("FixIt failed, requires manual intervention: ");
        if (!l.second.empty())
            msg += ' ' + l.second;

        reallyEmitWarning(l.first, msg + tag, {});
    }

    m_queuedManualInterventionWarnings.clear();
}

void CheckBase::reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const vector<FixItHint> &fixits)
{
    FullSourceLoc full(loc, m_ci.getSourceManager());
    unsigned id = m_ci.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, error.c_str());
    DiagnosticBuilder B = m_ci.getDiagnostics().Report(full, id);
    for (FixItHint fixit : fixits) {
        if (!fixit.isNull())
            B.AddFixItHint(fixit);
    }
}

void CheckBase::queueManualFixitWarning(clang::SourceLocation loc, int fixitType, const string &message)
{
    if (isFixitEnabled(fixitType) && !manualFixitAlreadyQueued(loc)) {
        m_queuedManualInterventionWarnings.push_back({loc, message});
        m_emittedManualFixItsWarningsInMacro.push_back(loc.getRawEncoding());
    }
}

bool CheckBase::warningAlreadyEmitted(SourceLocation loc) const
{
    PresumedLoc ploc = m_ci.getSourceManager().getPresumedLoc(loc);
    for (auto rawLoc : m_emittedWarningsInMacro) {
        SourceLocation l = SourceLocation::getFromRawEncoding(rawLoc);
        PresumedLoc p = m_ci.getSourceManager().getPresumedLoc(l);
        if (Utils::presumedLocationsEqual(p, ploc))
            return true;
    }

    return false;
}

bool CheckBase::manualFixitAlreadyQueued(SourceLocation loc) const
{
    PresumedLoc ploc = m_ci.getSourceManager().getPresumedLoc(loc);
    for (auto loc : m_emittedManualFixItsWarningsInMacro) {
        SourceLocation l = SourceLocation::getFromRawEncoding(loc);
        PresumedLoc p = m_ci.getSourceManager().getPresumedLoc(l);
        if (Utils::presumedLocationsEqual(p, ploc))
            return true;
    }

    return false;
}

std::vector<string> CheckBase::supportedOptions() const
{
    return {};
}

bool CheckBase::isOptionSet(const std::string &optionName) const
{
    const string qualifiedName = name() + '-' + optionName;
    return CheckManager::instance()->isOptionSet(qualifiedName);
}

void CheckBase::setEnabledFixits(int fixits)
{
    m_enabledFixits = fixits;
}

bool CheckBase::isFixitEnabled(int fixit) const
{
    return (m_enabledFixits & fixit) || CheckManager::instance()->allFixitsEnabled();
}
