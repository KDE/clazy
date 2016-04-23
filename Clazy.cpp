/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "Utils.h"
#include "StringUtils.h"
#include "clazy_stl.h"
#include "checkbase.h"
#include "checkmanager.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Frontend/FixItRewriter.h"
#include "clang/AST/ParentMap.h"
#include <llvm/Config/llvm-config.h>

#include <stdio.h>
#include <sstream>
#include <iostream>

using namespace clang;
using namespace std;

namespace {


class MyFixItOptions : public FixItOptions
{
public:
    MyFixItOptions(const MyFixItOptions &other) = delete;
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
        return InPlace ? filename : filename + "_fixed.cpp";
    }

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
    // Clang >= 3.7 already has this member.
    // We define it for clang <= 3.6 so it builds.
    bool InPlace;
#endif
};

static void manuallyPopulateParentMap(ParentMap *map, Stmt *s)
{
    if (!s)
        return;

    for (Stmt *child : s->children()) {
        llvm::errs() << "Patching " << child->getStmtClassName() << "\n";
        map->setParent(child, s);
        manuallyPopulateParentMap(map, child);
    }
}

class LazyASTConsumer : public ASTConsumer, public RecursiveASTVisitor<LazyASTConsumer>
{
public:
    LazyASTConsumer(CompilerInstance &ci, const RegisteredCheck::List &requestedChecks, bool inplaceFixits)
        : m_ci(ci)
        , m_rewriter(nullptr)
        , m_parentMap(nullptr)
    {
        m_createdChecks = CheckManager::instance()->createChecks(requestedChecks, ci);
        if (CheckManager::instance()->fixitsEnabled())
            m_rewriter = new FixItRewriter(ci.getDiagnostics(), m_ci.getSourceManager(), m_ci.getLangOpts(), new MyFixItOptions(inplaceFixits));
    }

    ~LazyASTConsumer()
    {
        if (m_rewriter) {
            m_rewriter->WriteFixedFiles();
            delete m_rewriter;
        }

        delete m_parentMap;
    }

    void setParentMap(ParentMap *map)
    {
        assert(map && !m_parentMap);
        m_parentMap = map;
        for (auto &check : m_createdChecks)
            check->setParentMap(map);
    }

    bool VisitDecl(Decl *decl)
    {
        for (const auto &check : m_createdChecks) {
            check->VisitDeclaration(decl);
        }

        return true;
    }

    bool VisitStmt(Stmt *stm)
    {
        if (!m_parentMap) {
            if (m_ci.getDiagnostics().hasUnrecoverableErrorOccurred())
                return false; // ParentMap sometimes crashes when there were errors. Doesn't like a botched AST.

            setParentMap(new ParentMap(stm));
        }

        // Workaround llvm bug: Crashes creating a parent map when encountering Catch Statements.
        if (lastStm && isa<CXXCatchStmt>(lastStm) && !m_parentMap->hasParent(stm)) {
            m_parentMap->setParent(stm, lastStm);
            manuallyPopulateParentMap(m_parentMap, stm);
        }

        lastStm = stm;

        // clang::ParentMap takes a root statement, but there's no root statement in the AST, the root is a declaration
        // So add to parent map each time we go into a different hierarchy
        if (!m_parentMap->hasParent(stm))
            m_parentMap->addStmt(stm);

        for (const auto &check : m_createdChecks) {
            check->VisitStatement(stm);
        }

        return true;
    }

    void HandleTranslationUnit(ASTContext &ctx) override
    {
        TraverseDecl(ctx.getTranslationUnitDecl());
    }

    Stmt *lastStm = nullptr;
    CompilerInstance &m_ci;
    FixItRewriter *m_rewriter;
    ParentMap *m_parentMap;
    CheckBase::List m_createdChecks;
};

//------------------------------------------------------------------------------

static bool parseArgument(const string &arg, vector<string> &args)
{
    auto it = clazy_std::find(args, arg);
    if (it != args.end()) {
        args.erase(it, it + 1);
        return true;
    }

    return false;
}

static CheckLevel parseLevel(vector<std::string> &args)
{
    static const vector<string> levels = { "level0", "level1", "level2", "level3", "level4" };
    const int numLevels = levels.size();
    for (int i = 0; i < numLevels; ++i) {
        if (parseArgument(levels.at(i), args)) {
            return static_cast<CheckLevel>(i);
        }
    }

    return CheckLevelUndefined;
}

static bool checkLessThan(const RegisteredCheck &c1, const RegisteredCheck &c2)
{
    return c1.name < c2.name;
}

static bool checkLessThanByLevel(const RegisteredCheck &c1, const RegisteredCheck &c2)
{
    return c1.level < c2.level;
}

class LazyASTAction : public PluginASTAction {
protected:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &ci, llvm::StringRef) override
    {
        return llvm::make_unique<LazyASTConsumer>(ci, m_checks, m_inplaceFixits);
    }

    bool ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args_) override
    {
        std::vector<std::string> args = args_;

        if (parseArgument("help", args)) {
            PrintHelp(llvm::errs());
            return true;
        }

        if (parseArgument("no-inplace-fixits", args)) {
            // Unit-tests don't use inplace fixits
            m_inplaceFixits = false;
        }

        // This argument is for debugging purposes
        const bool printRequestedChecks = parseArgument("print-requested-checks", args);

        auto checkManager = CheckManager::instance();
        const CheckLevel requestedLevel = parseLevel(/*by-ref*/args);
        if (requestedLevel != CheckLevelUndefined) {
            checkManager->setRequestedLevel(requestedLevel);
        }

        if (parseArgument("enable-all-fixits", args)) {
            // This is useful for unit-tests, where we also want to run fixits. Don't use it otherwise.
            checkManager->enableAllFixIts();
        }

        if (args.size() > 1) {
            // Too many arguments.
            llvm::errs() << "Too many arguments: ";
            for (const std::string &a : args)
                llvm::errs() << a << ' ';
            llvm::errs() << "\n";

            PrintHelp(llvm::errs());
            return false;
        } else if (args.size() == 1) {
            m_checks = checkManager->checksForCommaSeparatedString(args[0]);
            if (m_checks.empty()) {
                llvm::errs() << "Could not find checks in comma separated string " + args[0] + "\n";
                PrintHelp(llvm::errs());
                return false;
            }
        }

        // Append checks specified from env variable
        RegisteredCheck::List checksFromEnv = CheckManager::instance()->requestedChecksThroughEnv();
        copy(checksFromEnv.cbegin(), checksFromEnv.cend(), back_inserter(m_checks));

        if (m_checks.empty() && requestedLevel == CheckLevelUndefined) {
            // No check or level specified, lets use the default level
            checkManager->setRequestedLevel(DefaultCheckLevel);
        }

        // Add checks from requested level
        auto checksFromRequestedLevel = checkManager->checksFromRequestedLevel();
        clazy_std::append(checksFromRequestedLevel, m_checks);
        clazy_std::sort_and_remove_dups(m_checks, checkLessThan);

        if (printRequestedChecks) {
            llvm::errs() << "Requested checks: ";
            const unsigned int numChecks = m_checks.size();
            for (unsigned int i = 0; i < numChecks; ++i) {
                llvm::errs() << m_checks.at(i).name;
                const bool isLast = i == numChecks - 1;
                if (!isLast) {
                    llvm::errs() << ", ";
                }
            }

            llvm::errs() << "\n";
        }

        return true;
    }

    void PrintHelp(llvm::raw_ostream &ros)
    {
        RegisteredCheck::List checks = CheckManager::instance()->availableChecks(MaxCheckLevel);
        clazy_std::sort(checks, checkLessThanByLevel);

        ros << "Available checks and FixIts:\n\n";

        int lastPrintedLevel = -1;
        const auto numChecks = checks.size();
        for (unsigned int i = 0; i < numChecks; ++i) {
            const RegisteredCheck &check = checks[i];
            if (lastPrintedLevel < check.level) {
                lastPrintedLevel = check.level;

                if (check.level > 0)
                    ros << "\n";

                ros << "Checks from level" << to_string(check.level) << ":\n";
            }

            auto padded = check.name;
            padded.insert(padded.end(), 39 - padded.size(), ' ');
            ros << "    " << check.name;
            auto fixits = CheckManager::instance()->availableFixIts(check.name);
            if (!fixits.empty()) {
                ros << "    (";
                bool isFirst = true;
                for (const auto& fixit : fixits) {
                    if (isFirst) {
                        isFirst = false;
                    } else {
                        ros << ',';
                    }

                    ros << fixit.name;
                }
                ros << ')';
            }
            ros << "\n";
        }
        ros << "\nIf nothing is specified, all checks from level0 and level1 will be run.\n\n";
        ros << "To specify which checks to enable set the CLAZY_CHECKS env variable, for example:\n";
        ros << "    export CLAZY_CHECKS=\"level0\"\n";
        ros << "    export CLAZY_CHECKS=\"level0,reserve-candidates,qstring-allocations\"\n";
        ros << "    export CLAZY_CHECKS=\"reserve-candidates\"\n\n";
        ros << "or pass as compiler arguments, for example:\n";
        ros << "    -Xclang -plugin-arg-clang-lazy -Xclang reserve-candidates,qstring-allocations\n";
        ros << "\n";
        ros << "To enable FixIts for a check, also set the env variable CLAZY_FIXIT, for example:\n";
        ros << "    export CLAZY_FIXIT=\"fix-qlatin1string-allocations\"\n\n";
        ros << "FixIts are experimental and rewrite your code therefore only one FixIt is allowed per build.\nSpecifying a list of different FixIts is not supported.\nBackup your code before running them.\n";
    }

private:
    RegisteredCheck::List m_checks;
    bool m_inplaceFixits = true;
};

}

static FrontendPluginRegistry::Add<LazyASTAction>
X("clang-lazy", "clang lazy plugin");

#ifdef CLAZY_ON_WINDOWS_HACK
LLVM_EXPORT_REGISTRY(FrontendPluginRegistry)
#endif
