#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "checkbase.h"
#include "checks/level1/install-event-filter.h"
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
    explicit FullASTVisitor(ClazyContext &context, ClangTidyCheck &Check, ClangTidyContext *clangTidyCtx)
        : m_check(Check)
        , m_context(context)
        , m_checks({new InstallEventFilter("install-event-filter", &context, Check)})
        , clangTidyCtx(clangTidyCtx)

    {
    }

    ~FullASTVisitor()
    {
        std::for_each(m_checks.begin(), m_checks.end(), [](CheckBase *check) {
            delete check;
        });
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
        if (Utils::isMainFile(m_context.sm, stmt->getBeginLoc())) {
            auto &engine = m_context.astContext.getDiagnostics();
            const auto id = engine.getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, "oh no");
            // llvm::errs() << id << engine.getDiagnosticIDs()->getWarningOptionForDiag(id).str() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
            //  engine.Report(FullSourceLoc(stmt->getBeginLoc(), m_context.astContext.getSourceManager()), id);
            // m_check.diag(stmt->getBeginLoc(), "oh no");
            // clangTidyCtx->diag("testmeeeeeeeeeee", stmt->getBeginLoc(), "däääääämn", DiagnosticIDs::Warning);
        }
        return true;
    }
    bool VisitDecl(Decl *decl)
    {
        std::for_each(m_checks.begin(), m_checks.end(), [decl](CheckBase *check) {
            check->VisitDecl(decl);
        });
        return true;
    }

private:
    ClangTidyCheck &m_check;
    ClazyContext &m_context;
    std::vector<CheckBase *> m_checks;
    ClangTidyContext *clangTidyCtx;
};

class ClazyCheck : public ClangTidyCheck
{
public:
    ClazyCheck(StringRef CheckName, ClangTidyContext *Context)
        : ClangTidyCheck(CheckName, Context)
        , Context(Context)
    {
    }

    void registerMatchers(ast_matchers::MatchFinder *Finder) override
    {
        // Finder->addMatcher(stmt().bind("Stmt"), this);
        Finder->addMatcher(translationUnitDecl().bind("tu"), this);
    }

    void check(const ast_matchers::MatchFinder::MatchResult &Result) override
    {
        /*auto *stmt = Result.Nodes.getNodeAs<Stmt>("Stmt");
        if (!Utils::isMainFile(Result.Context->getSourceManager(), stmt->getBeginLoc())) {
            return;
        }*/

        ClazyContext ctx(*Result.Context, *m_pp, "", "", "", {}, {});
        FullASTVisitor visitor(ctx, *this, Context);
        // Result.Context->getDiagnostics().dump();
        auto translationUnit = const_cast<TranslationUnitDecl *>(Result.Nodes.getNodeAs<TranslationUnitDecl>("tu"));
        // translationUnit->getBeginLoc().dump(*Result.SourceManager);
        visitor.TraverseDecl(translationUnit);
    }

    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) override
    {
        m_pp = PP;
    }
    Preprocessor *m_pp;
    ClangTidyContext *Context;
};

/// Create a subclass of ClangTidyModule to register Clazy checks.
class ClazyModule : public ClangTidyModule
{
public:
    void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override
    {
        CheckFactories.registerCheck<ClazyCheck>("clazy-install-event-filter");
    }
};

namespace clang::tidy
{
static ClangTidyModuleRegistry::Add<ClazyModule> X("clazy-module", "Adds all Clazy checks to clang-tidy.");
volatile int ClazyModuleAnchorSource = 0;
}
