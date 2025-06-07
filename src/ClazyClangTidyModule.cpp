#include "HierarchyUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
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
    explicit FullASTVisitor(ASTContext &Context, ClangTidyCheck &Check)
        : Context(Context)
        , Check(Check)
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
        auto *memberCallExpr = dyn_cast<CXXMemberCallExpr>(stmt);
        if (!memberCallExpr || memberCallExpr->getNumArgs() != 1) {
            return true;
        }

        const FunctionDecl *func = memberCallExpr->getDirectCallee();
        if (!func || func->getQualifiedNameAsString() != "QObject::installEventFilter") {
            return true;
        }

        Expr *expr = memberCallExpr->getImplicitObjectArgument();
        if (!expr) {
            return true;
        }

        if (Stmt *firstChild = clazy::getFirstChildAtDepth(expr, 1); !firstChild || !isa<CXXThisExpr>(firstChild)) {
            return true;
        }

        const Expr *arg1 = memberCallExpr->getArg(0);
        arg1 = arg1 ? arg1->IgnoreCasts() : nullptr;

        const CXXRecordDecl *record = clazy::typeAsRecord(arg1);
        auto methods = Utils::methodsFromString(record, "eventFilter");

        for (auto *method : methods) {
            if (method->getQualifiedNameAsString() != "QObject::eventFilter") { // It overrides it, probably on purpose then, don't warn.
                return true;
            }
        }

        Check.diag(stmt->getBeginLoc(), "'this' should usually be the filter object, not the monitored one.");
        return true;
    }

private:
    ASTContext &Context;
    ClangTidyCheck &Check;
};

class ClazyCheck : public ClangTidyCheck
{
public:
    ClazyCheck(StringRef CheckName, ClangTidyContext *Context)
        : ClangTidyCheck(CheckName, Context)
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

        FullASTVisitor visitor(*Result.Context, *this);
        auto translationUnit = const_cast<TranslationUnitDecl *>(Result.Nodes.getNodeAs<TranslationUnitDecl>("tu"));
        translationUnit->dump();
        visitor.TraverseDecl(translationUnit);
    }

    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) override
    {
    }
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
