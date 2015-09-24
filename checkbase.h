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

#ifndef CHECK_BASE_H
#define CHECK_BASE_H

#include <clang/Frontend/CompilerInstance.h>
#include <string>

namespace clang {
class CXXMethodDecl;
class Stmt;
class Decl;
class ParentMap;
class TranslationUnitDecl;
class FixItHint;
class PresumedLoc;
class SourceLocation;
}

class CheckBase
{
public:
    typedef std::vector<std::unique_ptr<CheckBase> > List;
    explicit CheckBase(const std::string &name);
    ~CheckBase();

    void VisitStatement(clang::Stmt *stm);
    void VisitDeclaration(clang::Decl *stm);

    std::string name() const;

    void setParentMap(clang::ParentMap *parentMap);
    void setEnabledFixits(int);
    bool isFixitEnabled(int fixit) const;

protected:
    virtual void VisitStmt(clang::Stmt *stm);
    virtual void VisitDecl(clang::Decl *decl);
    bool shouldIgnoreFile(clang::SourceLocation) const;
    virtual std::vector<std::string> filesToIgnore() const;
    void emitWarning(clang::SourceLocation loc, std::string error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, std::string error, const std::vector<clang::FixItHint> &fixits, bool printWarningTag = true);
    void reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const std::vector<clang::FixItHint> &fixits);

    void queueManualFixitWarning(clang::SourceLocation loc, int fixitType, const std::string &message = {});
    bool warningAlreadyEmitted(clang::SourceLocation loc) const;
    bool manualFixitAlreadyQueued(clang::SourceLocation loc) const;

    clang::FixItHint createReplacement(const clang::SourceRange &range, const std::string &replacement);
    clang::FixItHint createInsertion(const clang::SourceLocation &start, const std::string &insertion);

    clang::CompilerInstance &m_ci;
    clang::TranslationUnitDecl *m_tu;
    clang::ParentMap *m_parentMap;
    clang::CXXMethodDecl *m_lastMethodDecl;
    std::string m_name;

    clang::Decl *m_lastDecl;
private:
    std::vector<uint> m_emittedWarningsInMacro;
    std::vector<uint> m_emittedManualFixItsWarningsInMacro;
    std::vector<std::pair<clang::SourceLocation, std::string>> m_queuedManualInterventionWarnings;
    int m_enabledFixits;
};

#endif
