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

#include "Utils.h"
#include "Clazy.h"
#include "clazy_stl.h"
#include "checkbase.h"
#include "AccessSpecifierManager.h"
#include "SourceCompatibilityHelpers.h"
#include "FixItExporter.h"

#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>

#include <stdlib.h>
#include <mutex>

using namespace clang;
using namespace std;
using namespace clang::ast_matchers;


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

ClazyASTConsumer::ClazyASTConsumer(ClazyContext *context)
    : m_context(context)
{
#ifndef CLAZY_DISABLE_AST_MATCHERS
    m_matchFinder = new clang::ast_matchers::MatchFinder();
#endif
}

void ClazyASTConsumer::addCheck(const std::pair<CheckBase *, RegisteredCheck> &check)
{
    CheckBase *checkBase = check.first;
#ifndef CLAZY_DISABLE_AST_MATCHERS
    checkBase->registerASTMatchers(*m_matchFinder);
#endif
    //m_createdChecks.push_back(checkBase);

    const RegisteredCheck &rcheck = check.second;

    if (rcheck.options & RegisteredCheck::Option_VisitsStmts)
        m_checksToVisitStmts.push_back(checkBase);

    if (rcheck.options & RegisteredCheck::Option_VisitsDecls)
        m_checksToVisitDecls.push_back(checkBase);

}

ClazyASTConsumer::~ClazyASTConsumer()
{
#ifndef CLAZY_DISABLE_AST_MATCHERS
    delete m_matchFinder;
#endif
    delete m_context;
}

bool ClazyASTConsumer::VisitDecl(Decl *decl)
{
    if (AccessSpecifierManager *a = m_context->accessSpecifierManager) // Needs to visit system headers too (qobject.h for example)
        a->VisitDeclaration(decl);

    const bool isTypeDefToVisit = m_context->visitsAllTypedefs() && isa<TypedefNameDecl>(decl);
    const SourceLocation locStart = clazy::getLocStart(decl);
    if (locStart.isInvalid() || (m_context->sm.isInSystemHeader(locStart) && !isTypeDefToVisit))
        return true;

    const bool isFromIgnorableInclude = m_context->ignoresIncludedFiles() && !Utils::isMainFile(m_context->sm, locStart);

    m_context->lastDecl = decl;

    if (auto fdecl = dyn_cast<FunctionDecl>(decl)) {
        m_context->lastFunctionDecl = fdecl;
        if (auto mdecl = dyn_cast<CXXMethodDecl>(fdecl))
            m_context->lastMethodDecl = mdecl;
    }

    for (CheckBase *check : m_checksToVisitDecls) {
        if (!(isFromIgnorableInclude && check->canIgnoreIncludes()))
            check->VisitDecl(decl);
    }

    return true;
}

bool ClazyASTConsumer::VisitStmt(Stmt *stm)
{
    const SourceLocation locStart = clazy::getLocStart(stm);
    if (locStart.isInvalid() || m_context->sm.isInSystemHeader(locStart))
        return true;

    if (!m_context->parentMap) {
        if (m_context->ci.getDiagnostics().hasUnrecoverableErrorOccurred())
            return false; // ParentMap sometimes crashes when there were errors. Doesn't like a botched AST.

        m_context->parentMap = new ParentMap(stm);
    }

    ParentMap *parentMap = m_context->parentMap;

    // Workaround llvm bug: Crashes creating a parent map when encountering Catch Statements.
    if (lastStm && isa<CXXCatchStmt>(lastStm) && !parentMap->hasParent(stm)) {
        parentMap->setParent(stm, lastStm);
        manuallyPopulateParentMap(parentMap, stm);
    }

    lastStm = stm;

    // clang::ParentMap takes a root statement, but there's no root statement in the AST, the root is a declaration
    // So add to parent map each time we go into a different hierarchy
    if (!parentMap->hasParent(stm))
        parentMap->addStmt(stm);

    const bool isFromIgnorableInclude = m_context->ignoresIncludedFiles() && !Utils::isMainFile(m_context->sm, locStart);
    for (CheckBase *check : m_checksToVisitStmts) {
        if (!(isFromIgnorableInclude && check->canIgnoreIncludes()))
            check->VisitStmt(stm);
    }

    return true;
}

void ClazyASTConsumer::HandleTranslationUnit(ASTContext &ctx)
{
    // FIXME: EndSourceFile() is called automatically, but not BeginsSourceFile()
    if (m_context->exporter)
        m_context->exporter->BeginSourceFile(clang::LangOptions());

    if ((m_context->options & ClazyContext::ClazyOption_OnlyQt) && !m_context->isQt())
        return;

    // Run our RecursiveAstVisitor based checks:
    TraverseDecl(ctx.getTranslationUnitDecl());

#ifndef CLAZY_DISABLE_AST_MATCHERS
    // Run our AstMatcher base checks:
    m_matchFinder->matchAST(ctx);
#endif
}

static bool parseArgument(const string &arg, vector<string> &args)
{
    auto it = clazy::find(args, arg);
    if (it != args.end()) {
        args.erase(it);
        return true;
    }

    return false;
}

ClazyASTAction::ClazyASTAction()
    : PluginASTAction()
    , m_checkManager(CheckManager::instance())
{
}

std::unique_ptr<clang::ASTConsumer> ClazyASTAction::CreateASTConsumer(CompilerInstance &, llvm::StringRef)
{
    // NOTE: This method needs to be kept reentrant (but not necessarily thread-safe)
    // Might be called from multiple threads via libclang, each thread operates on a different instance though

    std::lock_guard<std::mutex> lock(CheckManager::lock());

    auto astConsumer = std::unique_ptr<ClazyASTConsumer>(new ClazyASTConsumer(m_context));
    auto createdChecks = m_checkManager->createChecks(m_checks, m_context);
    for (auto check : createdChecks) {
        astConsumer->addCheck(check);
    }

    return std::unique_ptr<clang::ASTConsumer>(astConsumer.release());
}

static std::string getEnvVariable(const char *name)
{
    const char *result = getenv(name);
    if (result)
        return result;
    else return std::string();
}

bool ClazyASTAction::ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args_)
{
    // NOTE: This method needs to be kept reentrant (but not necessarily thread-safe)
    // Might be called from multiple threads via libclang, each thread operates on a different instance though

    std::vector<std::string> args = args_;

    const string headerFilter = getEnvVariable("CLAZY_HEADER_FILTER");
    const string ignoreDirs = getEnvVariable("CLAZY_IGNORE_DIRS");
    std::string exportFixesFilename;

    if (parseArgument("help", args)) {
        m_context = new ClazyContext(ci, headerFilter, ignoreDirs,
                                     exportFixesFilename, {}, ClazyContext::ClazyOption_None);
        PrintHelp(llvm::errs());
        return true;
    }

    if (parseArgument("export-fixes", args) || getenv("CLAZY_EXPORT_FIXES"))
        m_options |= ClazyContext::ClazyOption_ExportFixes;

    if (parseArgument("qt4-compat", args))
        m_options |= ClazyContext::ClazyOption_Qt4Compat;

    if (parseArgument("only-qt", args))
        m_options |= ClazyContext::ClazyOption_OnlyQt;

    if (parseArgument("qt-developer", args))
        m_options |= ClazyContext::ClazyOption_QtDeveloper;

    if (parseArgument("visit-implicit-code", args))
        m_options |= ClazyContext::ClazyOption_VisitImplicitCode;

    if (parseArgument("ignore-included-files", args))
        m_options |= ClazyContext::ClazyOption_IgnoreIncludedFiles;

    if (parseArgument("export-fixes", args))
        exportFixesFilename = args.at(0);

    m_context = new ClazyContext(ci, headerFilter, ignoreDirs, exportFixesFilename, {}, m_options);

    // This argument is for debugging purposes
    const bool dbgPrintRequestedChecks = parseArgument("print-requested-checks", args);

    {
        std::lock_guard<std::mutex> lock(CheckManager::lock());
        m_checks = m_checkManager->requestedChecks(args,
                                                   m_options & ClazyContext::ClazyOption_Qt4Compat);
    }

    if (args.size() > 1) {
        // Too many arguments.
        llvm::errs() << "Too many arguments: ";
        for (const std::string &a : args)
            llvm::errs() << a << ' ';
        llvm::errs() << "\n";

        PrintHelp(llvm::errs());
        return false;
    } else if (args.size() == 1 && m_checks.empty()) {
        // Checks were specified but couldn't be found
        llvm::errs() << "Could not find checks in comma separated string " + args[0] + "\n";
        PrintHelp(llvm::errs());
        return false;
    }

    if (dbgPrintRequestedChecks)
        printRequestedChecks();

    return true;
}

void ClazyASTAction::printRequestedChecks() const
{
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

void ClazyASTAction::PrintHelp(llvm::raw_ostream &ros) const
{
    std::lock_guard<std::mutex> lock(CheckManager::lock());
    RegisteredCheck::List checks = m_checkManager->availableChecks(MaxCheckLevel);

    clazy::sort(checks, checkLessThanByLevel);

    ros << "Available checks and FixIts:\n\n";

    int lastPrintedLevel = -1;
    const auto numChecks = checks.size();
    for (unsigned int i = 0; i < numChecks; ++i) {
        const RegisteredCheck &check = checks[i];
        const string levelStr = "level" + to_string(check.level);
        if (lastPrintedLevel < check.level) {
            lastPrintedLevel = check.level;

            if (check.level > 0)
                ros << "\n";

            ros << "- Checks from " << levelStr << ":\n";
        }

        const string relativeReadmePath = "src/checks/" + levelStr + "/README-" + check.name + ".md";

        auto padded = check.name;
        padded.insert(padded.end(), 39 - padded.size(), ' ');
        ros << "    - " << check.name;;
        auto fixits = m_checkManager->availableFixIts(check.name);
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
    ros << "    -Xclang -plugin-arg-clazy -Xclang reserve-candidates,qstring-allocations\n";
    ros << "\n";
}

ClazyStandaloneASTAction::ClazyStandaloneASTAction(const string &checkList,
                                                   const string &headerFilter,
                                                   const string &ignoreDirs,
                                                   const string &exportFixesFilename,
                                                   const std::vector<string> &translationUnitPaths,
                                                   ClazyContext::ClazyOptions options)
    : clang::ASTFrontendAction()
    , m_checkList(checkList.empty() ? "level1" : checkList)
    , m_headerFilter(headerFilter.empty() ? getEnvVariable("CLAZY_HEADER_FILTER") : headerFilter)
    , m_ignoreDirs(ignoreDirs.empty() ? getEnvVariable("CLAZY_IGNORE_DIRS") : ignoreDirs)
    , m_exportFixesFilename(exportFixesFilename)
    , m_translationUnitPaths(translationUnitPaths)
    , m_options(options)
{
}

unique_ptr<ASTConsumer> ClazyStandaloneASTAction::CreateASTConsumer(CompilerInstance &ci, llvm::StringRef)
{
    auto context = new ClazyContext(ci, m_headerFilter, m_ignoreDirs, m_exportFixesFilename, m_translationUnitPaths, m_options);
    auto astConsumer = new ClazyASTConsumer(context);

    auto cm = CheckManager::instance();

    vector<string> checks; checks.push_back(m_checkList);
    const bool qt4Compat = m_options & ClazyContext::ClazyOption_Qt4Compat;
    const RegisteredCheck::List requestedChecks = cm->requestedChecks(checks, qt4Compat);

    if (requestedChecks.size() == 0) {
        llvm::errs() << "No checks were requested!\n" << "\n";
        return nullptr;
    }

    auto createdChecks = cm->createChecks(requestedChecks, context);
    for (const auto &check : createdChecks) {
        astConsumer->addCheck(check);
    }

    return unique_ptr<ASTConsumer>(astConsumer);
}

volatile int ClazyPluginAnchorSource = 0;

static FrontendPluginRegistry::Add<ClazyASTAction>
X("clazy", "clang lazy plugin");
