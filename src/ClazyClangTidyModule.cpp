#include "ClazyContext.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "checkbase.h"
#include "checkmanager.h"
#include "checks/level0/no-module-include.h"
#include "checks/level0/qcolor-from-literal.h"
#include "checks/level0/qstring-arg.h"
#include "checks/level1/install-event-filter.h"
#include "checks/level1/non-pod-global-static.h"
#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
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
        std::for_each(m_checks.begin(), m_checks.end(), [stmt](CheckBase *check) {
            check->VisitStmt(stmt);
        });
        return true;
    }
    bool VisitDecl(Decl *decl)
    {
        m_context.lastDecl = decl;
        std::for_each(m_checks.begin(), m_checks.end(), [decl](CheckBase *check) {
            check->VisitDecl(decl);
        });
        return true;
    }

private:
    ClazyContext &m_context;
    std::vector<CheckBase *> m_checks;
};

class ClazyCheck : public ClangTidyCheck
{
public:
    ClazyCheck(StringRef CheckName, ClangTidyContext *Context)
        : ClangTidyCheck(CheckName, Context)
        , clangTidyContext(Context)
        , clazyContext(nullptr)
        , m_checks({
              new InstallEventFilter("install-event-filter", nullptr),
              new NonPodGlobalStatic("non-pod-global-static", nullptr),
              new QColorFromLiteral("non-pod-global-static", nullptr),
              new QStringArg("qstring-arg", nullptr),
              new NoModuleInclude("no-module-include", nullptr),
          })
    {
    }

    ~ClazyCheck()
    {
        std::for_each(m_checks.begin(), m_checks.end(), [](CheckBase *check) {
            delete check;
        });
        delete clazyContext;
    }

    void registerMatchers(ast_matchers::MatchFinder *Finder) override
    {
        llvm::errs() << "registermatchers\n";
        Finder->addMatcher(translationUnitDecl().bind("tu"), this);
        std::for_each(m_checks.begin(), m_checks.end(), [Finder](CheckBase *check) {
            check->registerASTMatchers(*Finder);
        });
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
        llvm::errs() << "ppcallbacks\n";
        clazyContext = new ClazyContext(nullptr, PP->getSourceManager(), PP->getLangOpts(), PP->getPreprocessorOpts(), "", "", "", {}, {}, emitDiagnostic);

        const auto checks = CheckManager::instance()->availableChecks(MaxCheckLevel);
        for (const auto check : m_checks) {
            check->m_context = clazyContext;

            const std::string checkName = check->name();
            const auto foundIt = std::find_if(checks.begin(), checks.end(), [&checkName](const RegisteredCheck &availableCheck) {
                return availableCheck.name == checkName;
            });
            if (foundIt->options & RegisteredCheck::Option_PreprocessorCallbacks) {
                llvm::errs() << "enable pp" << foundIt->name << "\n";
                check->enablePreProcessorCallbacks(*PP);
            }
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
    }
};

class ClazyModule : public ClangTidyModule
{
public:
    void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override
    {
        CheckFactories.registerCheck<ClazyCheck>("clazy");
        CheckFactories.registerCheck<NoopCheck>("clazy-install-event-filter");
        CheckFactories.registerCheck<NoopCheck>("clazy-non-pod-global-static");
        CheckFactories.registerCheck<NoopCheck>("clazy-qcolor-from-literal");
    }
};

namespace clang::tidy
{
static ClangTidyModuleRegistry::Add<ClazyModule> X("clazy-module", "Adds all Clazy checks to clang-tidy.");
volatile int ClazyModuleAnchorSource = 0;
}
