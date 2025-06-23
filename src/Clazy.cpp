/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "Clazy.h"
#include "AccessSpecifierManager.h"
#include "ClazyVisitHelper.h"
#include "FixItExporter.h"
#include "Utils.h"
#include "checkbase.h"
#include "clazy_stl.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <mutex>
#include <stdlib.h>

using namespace clang;
using namespace clang::ast_matchers;

ClazyASTConsumer::ClazyASTConsumer(ClazyContext *context)
    : m_context(context)
{
    m_matchFinder = new clang::ast_matchers::MatchFinder();
}

void ClazyASTConsumer::addCheck(const std::pair<CheckBase *, RegisteredCheck> &check)
{
    CheckBase *checkBase = check.first;
    m_visitors.addCheck(check.second.options, checkBase);
    checkBase->registerASTMatchers(*m_matchFinder);
}

ClazyASTConsumer::~ClazyASTConsumer()
{
    delete m_matchFinder;
    delete m_context;
}

bool ClazyASTConsumer::VisitDecl(Decl *decl)
{
    return clazy::VisitHelper::VisitDecl(decl, m_context, m_visitors);
}

bool ClazyASTConsumer::VisitStmt(Stmt *stmt)
{
    return clazy::VisitHelper::VisitStmt(stmt, m_context, m_visitors);
}

void ClazyASTConsumer::HandleTranslationUnit(ASTContext &ctx)
{
    // FIXME: EndSourceFile() is called automatically, but not BeginsSourceFile()
    if (m_context->exporter) {
        m_context->exporter->BeginSourceFile(clang::LangOptions());
    }

    if ((m_context->options & ClazyContext::ClazyOption_OnlyQt) && !m_context->isQt()) {
        return;
    }

    // Run our RecursiveAstVisitor based checks:
    TraverseDecl(ctx.getTranslationUnitDecl());

    // Run our AstMatcher base checks:
    m_matchFinder->matchAST(ctx);
}

static bool parseArgument(const std::string &arg, std::vector<std::string> &args)
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

std::unique_ptr<clang::ASTConsumer> ClazyASTAction::CreateASTConsumer(CompilerInstance &ci, llvm::StringRef)
{
    // NOTE: This method needs to be kept reentrant (but not necessarily thread-safe)
    // Might be called from multiple threads via libclang, each thread operates on a different instance though

    std::lock_guard<std::mutex> lock(CheckManager::lock());

    auto astConsumer = std::unique_ptr<ClazyASTConsumer>(new ClazyASTConsumer(m_context));
    for (const auto &requestedCheck : m_checks) {
        auto *check = requestedCheck.factory(m_context);
        if (requestedCheck.options & RegisteredCheck::Option_PreprocessorCallbacks) {
            check->enablePreProcessorCallbacks(ci.getPreprocessor());
        }
        astConsumer->addCheck({check, requestedCheck});
    }

    return std::unique_ptr<clang::ASTConsumer>(astConsumer.release());
}

static std::string getEnvVariable(const char *name)
{
    const char *result = getenv(name);
    if (result) {
        return result;
    }
    return std::string();
}

bool ClazyASTAction::ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args_)
{
    // NOTE: This method needs to be kept reentrant (but not necessarily thread-safe)
    // Might be called from multiple threads via libclang, each thread operates on a different instance though

    std::vector<std::string> args = args_;

    const std::string headerFilter = getEnvVariable("CLAZY_HEADER_FILTER");
    const std::string ignoreDirs = getEnvVariable("CLAZY_IGNORE_DIRS");
    std::string exportFixesFilename;

    if (parseArgument("help", args)) {
        m_context = new ClazyContext(&ci.getASTContext(),
                                     ci.getSourceManager(),
                                     ci.getASTContext().getLangOpts(),
                                     ci.getPreprocessor().getPreprocessorOpts(),
                                     headerFilter,
                                     ignoreDirs,
                                     exportFixesFilename,
                                     {},
                                     ClazyContext::ClazyOption_None);
        m_context->registerPreprocessorCallbacks(ci.getPreprocessor());
        PrintHelp(llvm::errs());
        return true;
    }

    if (parseArgument("export-fixes", args) || getenv("CLAZY_EXPORT_FIXES")) {
        m_options |= ClazyContext::ClazyOption_ExportFixes;
    }

    if (parseArgument("only-qt", args)) {
        m_options |= ClazyContext::ClazyOption_OnlyQt;
    }

    if (parseArgument("qt-developer", args)) {
        m_options |= ClazyContext::ClazyOption_QtDeveloper;
    }

    if (parseArgument("visit-implicit-code", args)) {
        m_options |= ClazyContext::ClazyOption_VisitImplicitCode;
    }

    if (parseArgument("ignore-included-files", args)) {
        m_options |= ClazyContext::ClazyOption_IgnoreIncludedFiles;
    }

    if (parseArgument("export-fixes", args)) {
        exportFixesFilename = args.at(0);
    }

    m_context = new ClazyContext(&ci.getASTContext(),
                                 ci.getSourceManager(),
                                 ci.getASTContext().getLangOpts(),
                                 ci.getPreprocessor().getPreprocessorOpts(),
                                 headerFilter,
                                 ignoreDirs,
                                 exportFixesFilename,
                                 {},
                                 m_options);
    m_context->registerPreprocessorCallbacks(ci.getPreprocessor());

    // This argument is for debugging purposes
    const bool dbgPrintRequestedChecks = parseArgument("print-requested-checks", args);

    {
        std::lock_guard<std::mutex> lock(CheckManager::lock());
        m_checks = m_checkManager->requestedChecks(args);
    }

    if (args.size() > 1) {
        // Too many arguments.
        llvm::errs() << "Too many arguments: ";
        for (const std::string &a : args) {
            llvm::errs() << a << ' ';
        }
        llvm::errs() << "\n";

        PrintHelp(llvm::errs());
        return false;
    }
    if (args.size() == 1 && m_checks.empty()) {
        // Checks were specified but couldn't be found
        llvm::errs() << "Could not find checks in comma separated string " + args[0] + "\n";
        PrintHelp(llvm::errs());
        return false;
    }

    if (dbgPrintRequestedChecks) {
        printRequestedChecks();
    }

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
        const std::string levelStr = "level" + std::to_string(check.level);
        if (lastPrintedLevel < check.level) {
            lastPrintedLevel = check.level;

            if (check.level > 0) {
                ros << "\n";
            }

            ros << "- Checks from " << levelStr << ":\n";
        }

        auto padded = check.name;
        padded.insert(padded.end(), 39 - padded.size(), ' ');
        ros << "    - " << check.name;
        ;
        auto fixits = m_checkManager->availableFixIts(check.name);
        if (!fixits.empty()) {
            ros << "    (";
            bool isFirst = true;
            for (const auto &fixit : fixits) {
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

ClazyStandaloneASTAction::ClazyStandaloneASTAction(const std::string &checkList,
                                                   const std::string &headerFilter,
                                                   const std::string &ignoreDirs,
                                                   const std::string &exportFixesFilename,
                                                   const std::vector<std::string> &translationUnitPaths,
                                                   ClazyContext::ClazyOptions options)
    : m_checkList(checkList.empty() ? "level1" : checkList)
    , m_headerFilter(headerFilter.empty() ? getEnvVariable("CLAZY_HEADER_FILTER") : headerFilter)
    , m_ignoreDirs(ignoreDirs.empty() ? getEnvVariable("CLAZY_IGNORE_DIRS") : ignoreDirs)
    , m_exportFixesFilename(exportFixesFilename)
    , m_translationUnitPaths(translationUnitPaths)
    , m_options(options)
{
}

std::unique_ptr<ASTConsumer> ClazyStandaloneASTAction::CreateASTConsumer(CompilerInstance &ci, llvm::StringRef)
{
    auto *context = new ClazyContext(&ci.getASTContext(),
                                     ci.getSourceManager(),
                                     ci.getASTContext().getLangOpts(),
                                     ci.getPreprocessor().getPreprocessorOpts(),
                                     m_headerFilter,
                                     m_ignoreDirs,
                                     m_exportFixesFilename,
                                     m_translationUnitPaths,
                                     m_options);
    context->registerPreprocessorCallbacks(ci.getPreprocessor());
    auto *astConsumer = new ClazyASTConsumer(context);

    auto *cm = CheckManager::instance();

    std::vector<std::string> checks;
    checks.push_back(m_checkList);
    const RegisteredCheck::List requestedChecks = cm->requestedChecks(checks);

    if (requestedChecks.empty()) {
        llvm::errs() << "No checks were requested!\n"
                     << "\n";
        return nullptr;
    }

    for (const auto &requestedCheck : requestedChecks) {
        auto *check = requestedCheck.factory(context);
        if (requestedCheck.options & RegisteredCheck::Option_PreprocessorCallbacks) {
            check->enablePreProcessorCallbacks(ci.getPreprocessor());
        }
        astConsumer->addCheck({check, requestedCheck});
    }

    return std::unique_ptr<ASTConsumer>(astConsumer);
}

volatile int ClazyPluginAnchorSource = 0;

static FrontendPluginRegistry::Add<ClazyASTAction> X("clazy", "clang lazy plugin");
