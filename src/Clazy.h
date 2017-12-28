/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015-2017 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_AST_ACTION_H
#define CLAZY_AST_ACTION_H

#include "checkmanager.h"
#include "ClazyContext.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendAction.h>
#include "clang/AST/RecursiveASTVisitor.h"

#include <memory>
#include <vector>
#include <string>

namespace clang {
    class CompilerInstance;
}

/**
 * This is the FrontendAction that is run with clazy is used as a plugin.
 */
class ClazyASTAction : public clang::PluginASTAction
{
public:
    ClazyASTAction();

protected:
    /// @note This function is reentrant
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override;
    /// @note This function is reentrant
    bool ParseArgs(const clang::CompilerInstance &ci, const std::vector<std::string> &args_) override;

    void PrintHelp(llvm::raw_ostream &ros) const;
    void PrintAnchorHeader(llvm::raw_ostream &ro, RegisteredCheck::List &checks) const;
private:
    void printRequestedChecks() const;
    RegisteredCheck::List m_checks;
    ClazyContext::ClazyOptions m_options = 0;
    CheckManager *const m_checkManager;
    ClazyContext *m_context = nullptr;
};

/**
 * This is the FrontendAction that is run with clazy is used standalone instead of as a plugin.
 * i.e: when you run clazy-standalone, this is the invoked FrontendAction
 */
class ClazyStandaloneASTAction : public clang::ASTFrontendAction
{
public:
    explicit ClazyStandaloneASTAction(const std::string &checkList, ClazyContext::ClazyOptions = ClazyContext::ClazyOption_None);
protected:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override;
private:
    std::string m_checkList;
    const ClazyContext::ClazyOptions m_options;
};

/**
 * Clazy's AST Consumer.
 */
class ClazyASTConsumer : public clang::ASTConsumer,
                         public clang::RecursiveASTVisitor<ClazyASTConsumer>
{
public:
    explicit ClazyASTConsumer(ClazyContext *context);
    ~ClazyASTConsumer();
    bool shouldVisitImplicitCode() const { return m_context->isVisitImplicitCode(); }

    bool VisitDecl(clang::Decl *decl);
    bool VisitStmt(clang::Stmt *stm);
    void HandleTranslationUnit(clang::ASTContext &ctx) override;
    void addCheck(const std::pair<CheckBase *, RegisteredCheck> &check);

    ClazyContext *context() const { return m_context; }

private:
    ClazyASTConsumer(const ClazyASTConsumer &) = delete;
    clang::Stmt *lastStm = nullptr;
    ClazyContext *const m_context;
    //CheckBase::List m_createdChecks;
    CheckBase::List m_checksToVisitStmts;
    CheckBase::List m_checksToVisitDecls;
    clang::ast_matchers::MatchFinder m_matchFinder;
};

#endif
