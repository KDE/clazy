/*
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "ClazyVisitHelper.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "checkbase.h"
#include "checkmanager.h"
#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "clang/AST/ParentMap.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <utility>

using namespace clang::ast_matchers;
using namespace clang::tidy;
using namespace clang;

class FullASTVisitor : public RecursiveASTVisitor<FullASTVisitor>
{
public:
    explicit FullASTVisitor(ClazyContext *context,
                            ClangTidyCheck &Check,
                            const std::vector<CheckBase *> &checksToVisitStmt,
                            const std::vector<CheckBase *> &checksToVisitDecl)
        : m_context(context)
        , m_checksToVisitStmt(checksToVisitStmt)
        , m_checksToVisitDecl(checksToVisitDecl)

    {
    }

    ~FullASTVisitor()
    {
    }

    bool VisitStmt(Stmt *stmt)
    {
        return clazy::VisitHelper::VisitStmt(stmt, m_context, m_checksToVisitStmt);
    }

    bool VisitDecl(Decl *decl)
    {
        return clazy::VisitHelper::VisitDecl(decl, m_context, m_checksToVisitDecl);
    }

private:
    ClazyContext *const m_context;
    const std::vector<CheckBase *> m_checksToVisitStmt;
    const std::vector<CheckBase *> m_checksToVisitDecl;
};

std::vector<std::string> s_enabledChecks;

class ClazyCheck : public ClangTidyCheck
{
public:
    ClazyCheck(StringRef CheckName, ClangTidyContext *Context)
        : ClangTidyCheck(CheckName, Context)
        , m_shouldRunClazyChecks(s_enabledChecks.empty())
        , clangTidyContext(Context)
        , clazyContext(nullptr)
    {
        // so that we later know which check was registered
        s_enabledChecks.emplace_back(CheckName);
    }

    ~ClazyCheck()
    {
        for (auto &checkPair : m_allChecks) {
            delete checkPair.first;
        }
        m_allChecks.clear();
        delete clazyContext;
    }

    void registerMatchers(ast_matchers::MatchFinder *Finder) override
    {
        if (!m_shouldRunClazyChecks) {
            return;
        }

        Finder->addMatcher(translationUnitDecl().bind("tu"), this);
        const auto checks = CheckManager::instance()->availableChecks(ManualCheckLevel);
        for (const auto &availCheck : checks) {
            const std::string checkName = "clazy-" + availCheck.name;
            if (std::find(s_enabledChecks.begin(), s_enabledChecks.end(), checkName) == s_enabledChecks.end()) {
                continue;
            }
            auto *check = availCheck.factory(clazyContext);
            if (availCheck.options & RegisteredCheck::Option_VisitsStmts) {
                m_checksToVisitStmt.emplace_back(check);
            }
            if (availCheck.options & RegisteredCheck::Option_VisitsDecls) {
                m_checksToVisitDecl.emplace_back(check);
            }

            check->registerASTMatchers(*Finder);
            m_allChecks.emplace_back(std::pair{check, availCheck.options});
        }
    }

    void check(const ast_matchers::MatchFinder::MatchResult &Result) override
    {
        if (!m_shouldRunClazyChecks) {
            return;
        }
        clazyContext->astContext = Result.Context;

        FullASTVisitor visitor(clazyContext, *this, m_checksToVisitStmt, m_checksToVisitDecl);
        auto translationUnit = const_cast<TranslationUnitDecl *>(Result.Nodes.getNodeAs<TranslationUnitDecl>("tu"));
        visitor.TraverseDecl(translationUnit);
    }

    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) override
    {
        if (!m_shouldRunClazyChecks) {
            return;
        }
        clazyContext = new ClazyContext(nullptr, PP->getSourceManager(), getLangOpts(), PP->getPreprocessorOpts(), "", "", "", {}, {}, emitDiagnostic, true);
        clazyContext->registerPreprocessorCallbacks(*PP);

        for (const auto [check, options] : m_allChecks) {
            check->m_context = clazyContext;
            if (options & RegisteredCheck::Option_PreprocessorCallbacks) {
                check->enablePreProcessorCallbacks(*PP);
            }
        }
    }

    const bool m_shouldRunClazyChecks;

    ClangTidyContext *clangTidyContext;
    ClazyContext *clazyContext;
    std::vector<std::pair<CheckBase *, RegisteredCheck::Options>> m_allChecks;
    std::vector<CheckBase *> m_checksToVisitStmt;
    std::vector<CheckBase *> m_checksToVisitDecl;
    // setting the engine fixes a weird crash, but we still run in a codepath where we do not know the check name in the end
    const ClazyContext::WarningReporter emitDiagnostic = //
        [this](const std::string &checkName,
               const clang::SourceLocation &loc,
               clang::DiagnosticIDs::Level level,
               std::string error,
               const std::vector<clang::FixItHint> &fixits) {
            clangTidyContext->diag("clazy-" + checkName, loc, error, level) << fixits;
        };
};

class ClazyModule : public ClangTidyModule
{
public:
    void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override
    {
        for (const auto &check : CheckManager::instance()->availableChecks(CheckLevel::ManualCheckLevel)) {
            CheckFactories.registerCheck<ClazyCheck>("clazy-" + check.name);
        }
    }
};

namespace clang::tidy
{
static ClangTidyModuleRegistry::Add<ClazyModule> X("clazy-module", "Adds all Clazy checks to clang-tidy.");
volatile int ClazyModuleAnchorSource = 0;
}
