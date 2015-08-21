/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "checkbase.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMap.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>

#include <vector>

using namespace clang;
using namespace std;

CheckBase::CheckBase(const string &name)
    : m_ci(*CheckManager::instance()->m_ci)
    , m_name(name)
    , m_lastDecl(nullptr)
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

void CheckBase::emitWarning(clang::SourceLocation loc, std::string error, bool printWarningTag) const
{
    emitWarning(loc, error, {}, printWarningTag);
}

void CheckBase::emitWarning(clang::SourceLocation loc, std::string error, const vector<FixItHint> &fixits, bool printWarningTag) const
{
    if (printWarningTag)
        error += string(" [-Wmore-warnings-") + name() + string("]");

    FullSourceLoc full(loc, m_ci.getSourceManager());
    unsigned id = m_ci.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, error.c_str());
    DiagnosticBuilder B = m_ci.getDiagnostics().Report(full, id);
    for (FixItHint fixit : fixits) {
        if (!fixit.isNull())
            B.AddFixItHint(fixit);
    }
}

void CheckBase::emitManualFixitWarning(clang::SourceLocation loc)
{
    emitWarning(loc, "FixIt failed, requires manual intervention", {}, true);
}

bool CheckBase::locationAlreadyFixed(SourceLocation loc) const
{
    PresumedLoc ploc = m_ci.getSourceManager().getPresumedLoc(loc);
    for (auto rawLoc : m_fixitLocationHistory) {
        SourceLocation l = SourceLocation::getFromRawEncoding(rawLoc);
        PresumedLoc p = m_ci.getSourceManager().getPresumedLoc(l);
        if (p.getColumn() == ploc.getColumn() && p.getLine() == ploc.getLine() && string(p.getFilename()) == string(ploc.getFilename()))
            return true;
    }

    return false;
}

clang::FixItHint CheckBase::createReplacement(const SourceRange &range, const string &replacement)
{
    if (range.getBegin().isInvalid() || locationAlreadyFixed(range.getBegin())) {
        return {};
    } else {
        m_fixitLocationHistory.push_back(range.getBegin().getRawEncoding());
        return FixItHint::CreateReplacement(range, replacement);
    }
}

clang::FixItHint CheckBase::createInsertion(const SourceLocation &start, const string &insertion)
{
    if (start.isInvalid() || locationAlreadyFixed(start)) {
        return {};
    } else {
        StringUtils::printLocation(start);
        m_fixitLocationHistory.push_back(start.getRawEncoding());
        return FixItHint::CreateInsertion(start, insertion);
    }
}
