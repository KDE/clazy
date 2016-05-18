/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015-2016 Sergio Martins <smartins@kde.org>

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

#ifndef CHECK_BASE_H
#define CHECK_BASE_H

#include "clazylib_export.h"

#include "clazy_stl.h"
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Config/llvm-config.h>
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

class CheckManager;
class SuppressionManager;

enum CheckLevel {
    CheckLevelUndefined = -1,
    CheckLevel0 = 0, // 100% safe, no false-positives, very useful
    CheckLevel1,     // Similar to CheckLevel0, but sometimes (rarely) there might be some false positive
    CheckLevel2,     // Sometimes has false-positives (20-30%)
    CheckLevel3 = 3, // Not always correct, possibly very noisy, requires a knowledgeable developer to review, might have a very big rate of false-positives
    HiddenCheckLevel, // The check is hidden and must be explicitly enabled
    MaxCheckLevel = CheckLevel3,
    DefaultCheckLevel = CheckLevel1
};

class CLAZYLIB_EXPORT CheckBase
{
public:
    typedef std::vector<std::unique_ptr<CheckBase> > List;
    explicit CheckBase(const std::string &name, const clang::CompilerInstance &ci);
    CheckBase(const CheckBase &other) = delete;

    virtual ~CheckBase();

    void VisitStatement(clang::Stmt *stm);
    void VisitDeclaration(clang::Decl *stm);

    std::string name() const;

    void setParentMap(clang::ParentMap *parentMap);
    void setEnabledFixits(int);
    bool isFixitEnabled(int fixit) const;

    void emitWarning(clang::Decl *, const std::string &error, bool printWarningTag = true);
    void emitWarning(clang::Stmt *, const std::string &error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, std::string error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, std::string error, const std::vector<clang::FixItHint> &fixits, bool printWarningTag = true);

protected:
    virtual void VisitStmt(clang::Stmt *stm);
    virtual void VisitDecl(clang::Decl *decl);
    bool shouldIgnoreFile(clang::SourceLocation) const;
    virtual bool ignoresAstNodesInSystemHeaders() const { return true; }
    virtual std::vector<std::string> filesToIgnore() const;
    void reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const std::vector<clang::FixItHint> &fixits);

    void queueManualFixitWarning(clang::SourceLocation loc, int fixitType, const std::string &message = {});
    bool warningAlreadyEmitted(clang::SourceLocation loc) const;
    bool manualFixitAlreadyQueued(clang::SourceLocation loc) const;
    virtual std::vector<std::string> supportedOptions() const;
    bool isOptionSet(const std::string &optionName) const;

    // 3 shortcuts for stuff that litter the codebase all over.
    const clang::CompilerInstance &ci() const { return m_ci; }
    const clang::SourceManager &sm() const { return m_ci.getSourceManager(); }
    const clang::LangOptions &lo() const { return m_ci.getLangOpts(); }

    const clang::CompilerInstance &m_ci;
    const std::string m_name;
    clang::ASTContext &m_context;
    clang::TranslationUnitDecl *const m_tu;
    clang::ParentMap *m_parentMap;

    clang::CXXMethodDecl *m_lastMethodDecl = nullptr;
    clang::Decl *m_lastDecl = nullptr;
    clang::Stmt *m_lastStmt = nullptr;
    SuppressionManager *m_suppressionManager = nullptr;
private:
    std::vector<unsigned int> m_emittedWarningsInMacro;
    std::vector<unsigned int> m_emittedManualFixItsWarningsInMacro;
    std::vector<std::pair<clang::SourceLocation, std::string>> m_queuedManualInterventionWarnings;
    int m_enabledFixits;
    CheckManager *const m_checkManager;
};

#endif
