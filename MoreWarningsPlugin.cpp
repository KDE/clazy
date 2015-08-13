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
#include "checks/missingtypeinfo.h"
#include "checks/nonpodstatic.h"
#include "checks/nrvoenabler.h"
#include "checks/assertwithsideeffects.h"
#include "checks/qlistint.h"
#include "checks/qmapkey.h"
#include "checks/requiredresults.h"
#include "checks/reserveadvisor.h"
#include "checks/variantsanitizer.h"
#include "checks/virtualcallsfromctor.h"
#include "checks/qstringuneededheapallocations.h"

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
using namespace std;

namespace {

enum Check {
    InvalidCheck = -1, // Don't change order
    DetachingTemporariesCheck = 0,
    MissingTypeinfoCheck,
    BogusDynamicCastCheck,
    NonPodStaticCheck,
    ReserveCandidatesCheck,
    VariantSanitizerCheck,
    QMapPointerKeyCheck,
    ForeacherCheck,
    VirtualCallsFromCTORCheck,
    GlobalConstCharPointerCheck,
    FunctionArgsByRefCheck,
    InefficientQListCheck,
    QStringUneededHeapAllocationsCheck,
    LastCheck
};

static const vector<string> & availableChecksStr()
{
    static const vector<string> texts = { "detaching-temporary",
                                          "missing-typeinfo",
                                          "bogus-dynamic-cast",
                                          "non-pod-global-static",
                                          "reserve-candidates",
                                          "variant-sanitizer",
                                          "qmap-with-key-pointer",
                                          "foreacher",
                                          "virtual-call-ctor",
                                          "global-const-char-pointer",
                                          "function-args-by-ref",
                                          "inefficient-qlist",
                                          "qstring-uneeded-heap-allocations"
                                        };

    assert(texts.size() == LastCheck);
    return texts;
}

static Check checkFromText(const std::string &checkStr)
{
    const vector<string> &checksStr = availableChecksStr();

    auto it = std::find(checksStr.cbegin(), checksStr.cend(), checkStr);
    return it == checksStr.cend() ? InvalidCheck : static_cast<Check>(it - checksStr.cbegin());
}

class MoreWarningsASTConsumer : public ASTConsumer, public RecursiveASTVisitor<MoreWarningsASTConsumer>
{
public:
    MoreWarningsASTConsumer(CompilerInstance &ci, const vector<Check> checks)
        : m_ci(ci)
    {
        for (uint i = 0; i < checks.size(); ++i) {
            switch (checks[i]) {
            case DetachingTemporariesCheck:
                m_checks.push_back(std::shared_ptr<DetachingTemporaries>(new DetachingTemporaries(ci)));
                break;
            case MissingTypeinfoCheck:
                m_checks.push_back(std::shared_ptr<MissingTypeinfo>(new MissingTypeinfo(ci)));
                break;
            case BogusDynamicCastCheck:
                m_checks.push_back(std::shared_ptr<BogusDynamicCast>(new BogusDynamicCast(ci)));
                break;
            case NonPodStaticCheck:
                m_checks.push_back(std::shared_ptr<NonPodStatic>(new NonPodStatic(ci)));
                break;
            case ReserveCandidatesCheck:
                m_checks.push_back(std::shared_ptr<ReserveAdvisor>(new ReserveAdvisor(ci)));
                break;
            case VariantSanitizerCheck:
                m_checks.push_back(std::shared_ptr<VariantSanitizer>(new VariantSanitizer(ci)));
                break;
            case QMapPointerKeyCheck:
                m_checks.push_back(std::shared_ptr<QMapKeyChecker>(new QMapKeyChecker(ci)));
                break;
            case ForeacherCheck:
                m_checks.push_back(std::shared_ptr<Foreacher>(new Foreacher(ci)));
                break;
            case VirtualCallsFromCTORCheck:
                m_checks.push_back(std::shared_ptr<VirtualCallsFromCTOR>(new VirtualCallsFromCTOR(ci)));
                break;
            case GlobalConstCharPointerCheck:
                m_checks.push_back(std::shared_ptr<GlobalConstCharPointer>(new GlobalConstCharPointer(ci)));
                break;
            case FunctionArgsByRefCheck:
                m_checks.push_back(std::shared_ptr<FunctionArgsByRef>(new FunctionArgsByRef(ci)));
                break;
            case InefficientQListCheck:
                m_checks.push_back(std::shared_ptr<InefficientQList>(new InefficientQList(ci)));
                break;
            case QStringUneededHeapAllocationsCheck:
                m_checks.push_back(std::shared_ptr<QStringUneededHeapAllocations>(new QStringUneededHeapAllocations(ci)));
                break;
            default:
                assert(false);
            }
        }

        // These are commented because they are either WIP or have to many false-positives
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
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &ci, llvm::StringRef) override
    {
        return llvm::make_unique<MoreWarningsASTConsumer>(ci, m_checks);
    }

    bool ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args) override
    {
        // No checks supplied, use all of them
        if (args.empty()) {
            for (int i = 0; i < LastCheck; ++i) {
                m_checks.push_back(static_cast<Check>(i));
            }
            return true;
        }

        if (args[0] == "help" || args.size() > 1) {
            PrintHelp(llvm::errs());
            return false;
        }

        vector<string> requestedChecks = Utils::splitString(args[0], ',');
        if (requestedChecks.empty()) {
            PrintHelp(llvm::errs());
            return false;
        }

        // Remove duplicates:
        sort(requestedChecks.begin(), requestedChecks.end());
        requestedChecks.erase(unique(requestedChecks.begin(), requestedChecks.end()), requestedChecks.end());

        m_checks.reserve(LastCheck);

        for (uint i = 0, e = requestedChecks.size(); i != e; ++i) {
            Check check = checkFromText(requestedChecks[i]);
            if (check == InvalidCheck) {
                llvm::errs() << "Invalid argument: " << requestedChecks[i] << "\n";
                return false;
            } else {
                m_checks.push_back(check);
            }
        }

        return true;
    }

    void PrintHelp(llvm::raw_ostream &ros)
    {
        const vector<string> &checksStr = availableChecksStr();
        ros << "Available plugins:\n\n";
        for (uint i = 1; i < checksStr.size(); ++i) {
            ros << checksStr[i] << "\n";
        }
    }

private:
    vector<Check> m_checks;
};

}

static FrontendPluginRegistry::Add<MoreWarningsAction>
X("more-warnings", "more warnings plugin");
