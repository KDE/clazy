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

#include "Utils.h"
#include "StringUtils.h"

#include "checkbase.h"
#include "checkmanager.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Frontend/FixItRewriter.h"
#include "clang/AST/ParentMap.h"

#include <stdio.h>
#include <sstream>
#include <iostream>

using namespace clang;
using namespace std;

namespace {


class MyFixItOptions : public FixItOptions
{
public:
    MyFixItOptions(bool inplace)
    {
        InPlace = inplace;
        FixWhatYouCan = true;
        FixOnlyWarnings = true;
        Silent = false;
    }

    std::string RewriteFilename(const std::string &filename, int &fd) override
    {
        fd = -1;
        return InPlace ? filename : filename + "_fixed";
    }
};

static void manuallyPopulateParentMap(ParentMap *map, Stmt *s)
{
    if (!s)
        return;

    auto it = s->child_begin();
    auto e = s->child_end();
    for (; it != e; ++it) {
        llvm::errs() << "Patching " << (*it)->getStmtClassName() << "\n";
        map->setParent(*it, s);
        manuallyPopulateParentMap(map, *it);
    }
}

class LazyASTConsumer : public ASTConsumer, public RecursiveASTVisitor<LazyASTConsumer>
{
public:
    LazyASTConsumer(CompilerInstance &ci, const vector<string> &requestedChecks, bool inplaceFixits)
        : m_ci(ci)
        , m_rewriter(nullptr)
        , m_parentMap(nullptr)
        , m_checkManager(CheckManager::instance())
    {
        m_checkManager->setCompilerInstance(&m_ci);
        m_checkManager->createChecks(requestedChecks);
        if (m_checkManager->fixitsEnabled())
            m_rewriter = new FixItRewriter(ci.getDiagnostics(), m_ci.getSourceManager(), m_ci.getLangOpts(), new MyFixItOptions(inplaceFixits));
    }

    ~LazyASTConsumer()
    {
        if (m_rewriter != nullptr) {
            m_rewriter->WriteFixedFiles();
            delete m_rewriter;
        }
    }

    void setParentMap(ParentMap *map)
    {
        delete m_parentMap;
        m_parentMap = map;
        auto &createdChecks = m_checkManager->createdChecks();
        for (auto &check : createdChecks)
            check->setParentMap(map);
    }

    bool VisitDecl(Decl *decl)
    {
        auto &createdChecks = m_checkManager->createdChecks();
        for (auto it = createdChecks.cbegin(), end = createdChecks.cend(); it != end; ++it) {
            (*it)->VisitDeclaration(decl);
        }

        return true;
    }

    bool VisitStmt(Stmt *stm)
    {
        static Stmt *lastStm = nullptr;
        // Workaround llvm bug: Crashes creating a parent map when encountering Catch Statements.
        if (lastStm && isa<CXXCatchStmt>(lastStm) && m_parentMap && !m_parentMap->hasParent(stm)) {
            m_parentMap->setParent(stm, lastStm);
            manuallyPopulateParentMap(m_parentMap, stm);
        }

        lastStm = stm;

        // clang::ParentMap takes a root statement, but there's no root statement in the AST, the root is a declaration
        // So re-set a parent map each time we go into a different hieararchy
        if (m_parentMap == nullptr || !m_parentMap->hasParent(stm)) {
            assert(stm != nullptr);
            setParentMap(new ParentMap(stm));
        }

        auto &createdChecks = m_checkManager->createdChecks();
        for (auto it = createdChecks.cbegin(), end = createdChecks.cend(); it != end; ++it) {
            (*it)->VisitStatement(stm);
        }

        return true;
    }

    void HandleTranslationUnit(ASTContext &ctx) override
    {
        TraverseDecl(ctx.getTranslationUnitDecl());
    }

    CompilerInstance &m_ci;
    FixItRewriter *m_rewriter;
    ParentMap *m_parentMap;
    CheckManager *m_checkManager;
};

//------------------------------------------------------------------------------

class LazyASTAction : public PluginASTAction {
protected:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &ci, llvm::StringRef) override
    {
        return llvm::make_unique<LazyASTConsumer>(ci, m_checks, m_inplaceFixits);
    }

    bool ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args_) override
    {
        std::vector<std::string> args = args_;

        if (std::find(args.cbegin(), args.cend(), "help") != args.cend()) {
            llvm::errs() << "Help:\n";
            PrintHelp(llvm::errs());
            return false;
        }

        auto it = std::find(args.cbegin(), args.cend(), "no-inplace-fixits");
        if (it != args.cend()) {
            m_inplaceFixits = false; // Unit-tests don't use inplace fixits
            args.erase(it, it + 1);
        }

        if (args.empty()) {
            // No check specified, check the env variable
            m_checks = CheckManager::instance()->requestedCheckNamesThroughEnv();
            if (m_checks.empty()) {
                llvm::errs() << "No requested checks!\n";
                //PrintHelp(llvm::errs());
                return true;
            }
        } else if (args.size() > 1) {
            // Too many arguments.
            llvm::errs() << "Too many arguments: ";
            for (const std::string &a : args)
                llvm::errs() << a << " ";
            llvm::errs() << "\n";

            PrintHelp(llvm::errs());
            return false;
        } else if (args.size() == 1) {
            m_checks = CheckManager::instance()->checkNamesForCommaSeparatedString(args[0]);
            if (m_checks.empty()) {
                llvm::errs() << "No requested checks!\n";
                PrintHelp(llvm::errs());
                return false;
            }
        }

        return true;
    }

    void PrintHelp(llvm::raw_ostream &ros)
    {
        const vector<string> &names = CheckManager::instance()->availableCheckNames(false);

        ros << "To specify which checks to enable set the CLAZY_CHECKS env variable, for example:\n";
        ros << "export CLAZY_CHECKS=\"reserve-candidates,qstring-uneeded-heap-allocations\"\n\n";
        ros << "or pass as compiler arguments, for example:\n";
        ros << "-Xclang -plugin-arg-clang-lazy -Xclang reserve-candidates,qstring-uneeded-heap-allocations\n";
        ros << "\n";
        ros << "To enable FixIts for a check, also set the env variable CLAZY_FIXIT, for example:\n";
        ros << "export CLAZY_FIXIT=\"fix-qlatin1string-allocations\"\n\n";
        ros << "FixIts are experimental and rewrite your code therefore only one FixIt is allowed per build.\nSpecifying a list of different FixIts is not supported.\n\n";

        ros << "Available checks and FixIts:\n\n";
        for (uint i = 1; i < names.size(); ++i) {
            auto padded = names[i];
            padded.insert(padded.end(), 39 - padded.size(), ' ');
            ros << names[i];
            auto fixits = CheckManager::instance()->availableFixIts(names[i]);
            if (!fixits.empty()) {
                ros << "    (";
                bool isFirst = true;
                for (auto fixit : fixits) {
                    if (isFirst) {
                        isFirst = false;
                    } else {
                        ros << ",";
                    }

                    ros << fixit.name;
                }
                ros << ")";
            }
            ros << "\n";
        }

        exit(-2);
    }

private:
    vector<string> m_checks;
    bool m_inplaceFixits = true;
};

}

static FrontendPluginRegistry::Add<LazyASTAction>
X("clang-lazy", "clang lazy plugin");
