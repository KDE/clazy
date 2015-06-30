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

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMap.h>

#include <vector>

using namespace clang;

CheckBase::CheckBase(CompilerInstance &ci)
    : m_ci(ci)
{
    ASTContext &context = m_ci.getASTContext();
    m_tu = context.getTranslationUnitDecl();
    //m_parentMap = new ParentMap(m_tu->getBody());
    m_lastMethodDecl = nullptr;
}

CheckBase::~CheckBase()
{
}

void CheckBase::VisitStatement(Stmt *stm)
{
    SourceManager &sm = m_ci.getSourceManager();
    if (!shouldIgnoreFile(sm.getFilename(stm->getLocStart())))
        VisitStmt(stm);
}

void CheckBase::VisitDeclaration(Decl *decl)
{
    SourceManager &sm = m_ci.getSourceManager();
    if (shouldIgnoreFile(sm.getFilename(decl->getLocStart())))
        return;

    auto mdecl = dyn_cast<CXXMethodDecl>(decl);
    if (mdecl)
        m_lastMethodDecl = mdecl;

    VisitDecl(decl);
}

void CheckBase::VisitStmt(Stmt *)
{
}

void CheckBase::VisitDecl(Decl *)
{
}

bool CheckBase::shouldIgnoreFile(const std::string &filename) const
{
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

void CheckBase::emitWarning(clang::SourceLocation loc, const char *error) const
{
    FullSourceLoc full(loc, m_ci.getSourceManager());
    unsigned id = m_ci.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, error);
    DiagnosticBuilder B = m_ci.getDiagnostics().Report(full, id);
}
