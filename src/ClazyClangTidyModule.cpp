#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
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

using namespace clang::ast_matchers;
using namespace clang::tidy;
using namespace clang;

class FullASTVisitor : public RecursiveASTVisitor<FullASTVisitor>
{
public:
    explicit FullASTVisitor(ClazyContext &context, ClangTidyCheck &Check, const std::vector<CheckBase *> &checks)
        : m_context(context)
        , m_checks(checks)

    {
        std::for_each(m_checks.begin(), m_checks.end(), [](CheckBase *check) { });
    }

    ~FullASTVisitor()
    {
    }

    bool VisitFunctionDecl(FunctionDecl *FD)
    {
        if (FD->hasBody()) {
            VisitStmt(FD->getBody());
        }
        return true;
    }

    bool VisitStmt(Stmt *stmt)
    {
        if (!m_context.parentMap) {
            if (m_context.astContext->getDiagnostics().hasUnrecoverableErrorOccurred()) {
                return false; // ParentMap sometimes crashes when there were errors. Doesn't like a botched AST.
            }

            m_context.parentMap = new ParentMap(stmt);
        }

        for (auto *check : m_checks) {
            check->VisitStmt(stmt);
        }
        return true;
    }
    bool VisitDecl(Decl *decl)
    {
        if (AccessSpecifierManager *a = m_context.accessSpecifierManager) { // Needs to visit system headers too (qobject.h for example)
            a->VisitDeclaration(decl);
        }

        m_context.lastDecl = decl;
        if (auto *fdecl = dyn_cast<FunctionDecl>(decl)) {
            m_context.lastFunctionDecl = fdecl;
            if (auto *mdecl = dyn_cast<CXXMethodDecl>(fdecl)) {
                m_context.lastMethodDecl = mdecl;
            }
        }

        for (auto *check : m_checks) {
            check->VisitDecl(decl);
        }
        return true;
    }

private:
    ClazyContext &m_context;
    std::vector<CheckBase *> m_checks;
};

std::vector<std::string> s_enabledChecks;

class ClazyCheck : public ClangTidyCheck
{
public:
    ClazyCheck(StringRef CheckName, ClangTidyContext *Context)
        : ClangTidyCheck(CheckName, Context)
        , clangTidyContext(Context)
        , clazyContext(nullptr)
    {
    }

    ~ClazyCheck()
    {
        for (auto *check : m_checks) {
            delete check;
        }
        delete clazyContext;
    }

    void registerMatchers(ast_matchers::MatchFinder *Finder) override
    {
        Finder->addMatcher(translationUnitDecl().bind("tu"), this);
        for (auto *check : m_checks) {
            check->registerASTMatchers(*Finder);
        }
    }

    void check(const ast_matchers::MatchFinder::MatchResult &Result) override
    {
        clazyContext->astContext = Result.Context;

        FullASTVisitor visitor(*clazyContext, *this, m_checks);
        auto translationUnit = const_cast<TranslationUnitDecl *>(Result.Nodes.getNodeAs<TranslationUnitDecl>("tu"));
        visitor.TraverseDecl(translationUnit);
    }

    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) override
    {
        clazyContext = new ClazyContext(nullptr, PP->getSourceManager(), getLangOpts(), PP->getPreprocessorOpts(), "", "", "", {}, {}, emitDiagnostic);
        clazyContext->registerPreprocessorCallbacks(*PP);

        const auto checks = CheckManager::instance()->availableChecks(ManualCheckLevel);
        for (const auto &availCheck : checks) {
            const std::string checkName = "clazy-" + availCheck.name;
            if (std::find(s_enabledChecks.begin(), s_enabledChecks.end(), checkName) == s_enabledChecks.end()) {
                continue;
            }
            auto *check = availCheck.factory(clazyContext);
            if (availCheck.options & RegisteredCheck::Option_PreprocessorCallbacks) {
                check->enablePreProcessorCallbacks(*PP);
            }
            m_checks.emplace_back(check);
        }
    }

    ClangTidyContext *clangTidyContext;
    ClazyContext *clazyContext;
    std::vector<CheckBase *> m_checks;
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

class NoopCheck : public ClangTidyCheck
{
public:
    NoopCheck(StringRef CheckName, ClangTidyContext *Context)
        : ClangTidyCheck(CheckName, Context)
    {
        // Just for sake of registering a check
        s_enabledChecks.emplace_back(CheckName);
    }
};

class ClazyModule : public ClangTidyModule
{
public:
    void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override
    {
        CheckFactories.registerCheck<ClazyCheck>("clazy");
        for (const auto &check : CheckManager::instance()->availableChecks(CheckLevel::ManualCheckLevel)) {
            CheckFactories.registerCheck<NoopCheck>("clazy-" + check.name);
        }
    }
};

namespace clang::tidy
{
static ClangTidyModuleRegistry::Add<ClazyModule> X("clazy-module", "Adds all Clazy checks to clang-tidy.");
volatile int ClazyModuleAnchorSource = 0;
}
