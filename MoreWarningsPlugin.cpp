/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "Utils.h"
#include "checkbase.h"
#include "checks/detachingtemporaries.h"
#include "checks/duplicateexpensivestatement.h"
#include "checks/dynamic_cast.h"
#include "checks/inefficientqlist.h"
#include "checks/foreacher.h"
#include "checks/functionargsbyref.h"
#include "checks/globalconstcharpointer.h"
#include "checks/movablecontainers.h"
#include "checks/nonpodstatic.h"
#include "checks/nrvoenabler.h"
#include "checks/assertwithsideeffects.h"
#include "checks/qlistint.h"
#include "checks/qmapkey.h"
#include "checks/requiredresults.h"
#include "checks/reserveadvisor.h"
#include "checks/variantsanitizer.h"
#include "checks/virtualcallsfromctor.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"

#include <stdio.h>
#include <sstream>
#include <iostream>

using namespace clang;

// TODO:
// * Use plugin arguments to choose which checks to enable
// * Unit-tests

namespace {

class MoreWarningsASTConsumer : public ASTConsumer, public RecursiveASTVisitor<MoreWarningsASTConsumer>
{
public:
    MoreWarningsASTConsumer(CompilerInstance &ci)
        : m_ci(ci)
    {
        m_checks.push_back(std::shared_ptr<DetachingTemporaries>(new DetachingTemporaries(ci)));
        m_checks.push_back(std::shared_ptr<MovableContainers>(new MovableContainers(ci)));
        m_checks.push_back(std::shared_ptr<BogusDynamicCast>(new BogusDynamicCast(ci)));
        m_checks.push_back(std::shared_ptr<NonPodStatic>(new NonPodStatic(ci)));
        m_checks.push_back(std::shared_ptr<ReserveAdvisor>(new ReserveAdvisor(ci)));
        m_checks.push_back(std::shared_ptr<VariantSanitizer>(new VariantSanitizer(ci)));
        m_checks.push_back(std::shared_ptr<QMapKeyChecker>(new QMapKeyChecker(ci)));
        m_checks.push_back(std::shared_ptr<Foreacher>(new Foreacher(ci)));
        m_checks.push_back(std::shared_ptr<VirtualCallsFromCTOR>(new VirtualCallsFromCTOR(ci)));
        m_checks.push_back(std::shared_ptr<GlobalConstCharPointer>(new GlobalConstCharPointer(ci)));
        m_checks.push_back(std::shared_ptr<FunctionArgsByRef>(new FunctionArgsByRef(ci)));

        // These are commented because they are either WIP or have to many false-positives
        /// m_checks.push_back(std::shared_ptr<InefficientQList>(new InefficientQList(ci)));
        /// m_checks.push_back(std::shared_ptr<NRVOEnabler>(new NRVOEnabler(ci)));
        /// m_checks.push_back(std::shared_ptr<RequiredResults>(new RequiredResults(ci)));
        /// m_checks.push_back(std::shared_ptr<ListInt>(new ListInt(ci)));
        /// m_checks.push_back(std::shared_ptr<DuplicateExpensiveStatement>(new DuplicateExpensiveStatement(ci)));
        /// m_checks.push_back(std::shared_ptr<AssertWithSideEffects>(new AssertWithSideEffects(ci)));
    }

    bool VisitDecl(Decl *decl)
    {
        auto it = m_checks.cbegin();
        auto end = m_checks.cend();
        for (; it != end; ++it) {
            (*it)->VisitDeclaration(decl);
        }

        return true;
    }

    bool VisitStmt(Stmt *stm)
    {
        auto it = m_checks.cbegin();
        auto end = m_checks.cend();
        for (; it != end; ++it) {
            (*it)->VisitStatement(stm);
        }

        return true;
    }

    void HandleTranslationUnit(ASTContext &ctx)
    {
        TraverseDecl(ctx.getTranslationUnitDecl());
    }

    CompilerInstance &m_ci;
    std::vector<std::shared_ptr<CheckBase> > m_checks;
};

//------------------------------------------------------------------------------

class MoreWarningsAction : public PluginASTAction {
protected:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef)
    {
        return llvm::make_unique<MoreWarningsASTConsumer>(CI);
    }

    bool ParseArgs(const CompilerInstance &CI,
                   const std::vector<std::string> &args) {
        for (unsigned i = 0, e = args.size(); i != e; ++i) {
            llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";

            // Example error handling.
            if (args[i] == "-an-error") {
                DiagnosticsEngine &D = CI.getDiagnostics();
                unsigned DiagID = D.getCustomDiagID(
                            DiagnosticsEngine::Error, "invalid argument '%0'");
                D.Report(DiagID);
                return false;
            }
        }
        if (args.size() && args[0] == "help")
            PrintHelp(llvm::errs());

        return true;
    }

    void PrintHelp(llvm::raw_ostream &ros)
    {
        ros << "Help for MoreWarningsPlugin plugin goes here\n";
    }
};

}

static FrontendPluginRegistry::Add<MoreWarningsAction>
X("more-warnings", "more warnings plugin");
