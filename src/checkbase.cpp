/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "checkbase.h"
#include "ClazyContext.h"
#include "SuppressionManager.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <vector>

namespace clang
{
class MacroArgs;
class Token;
} // namespace clang

using namespace clang;
using namespace clang::ast_matchers;

ClazyPreprocessorCallbacks::ClazyPreprocessorCallbacks(CheckBase *check)
    : check(check)
{
}

void ClazyPreprocessorCallbacks::MacroExpands(const Token &macroNameTok, const MacroDefinition &md, SourceRange range, const MacroArgs *)
{
    check->VisitMacroExpands(macroNameTok, range, md.getMacroInfo());
}

void ClazyPreprocessorCallbacks::Defined(const Token &macroNameTok, const MacroDefinition &, SourceRange range)
{
    check->VisitDefined(macroNameTok, range);
}

void ClazyPreprocessorCallbacks::Ifdef(SourceLocation loc, const Token &macroNameTok, const MacroDefinition &)
{
    check->VisitIfdef(loc, macroNameTok);
}

void ClazyPreprocessorCallbacks::Ifndef(SourceLocation loc, const Token &macroNameTok, const MacroDefinition &)
{
    check->VisitIfndef(loc, macroNameTok);
}

void ClazyPreprocessorCallbacks::If(SourceLocation loc, SourceRange conditionRange, PPCallbacks::ConditionValueKind conditionValue)
{
    check->VisitIf(loc, conditionRange, conditionValue);
}

void ClazyPreprocessorCallbacks::Elif(SourceLocation loc, SourceRange conditionRange, PPCallbacks::ConditionValueKind conditionValue, SourceLocation ifLoc)
{
    check->VisitElif(loc, conditionRange, conditionValue, ifLoc);
}

void ClazyPreprocessorCallbacks::Else(SourceLocation loc, SourceLocation ifLoc)
{
    check->VisitElse(loc, ifLoc);
}

void ClazyPreprocessorCallbacks::Endif(SourceLocation loc, SourceLocation ifLoc)
{
    check->VisitEndif(loc, ifLoc);
}

void ClazyPreprocessorCallbacks::MacroDefined(const Token &macroNameTok, const MacroDirective *)
{
    check->VisitMacroDefined(macroNameTok);
}

void ClazyPreprocessorCallbacks::InclusionDirective(clang::SourceLocation HashLoc,
                                                    const clang::Token &IncludeTok,
                                                    llvm::StringRef FileName,
                                                    bool IsAngled,
                                                    clang::CharSourceRange FilenameRange,
                                                    clazy::OptionalFileEntryRef File,
                                                    llvm::StringRef SearchPath,
                                                    llvm::StringRef RelativePath,
                                                    const clang::Module *SuggestedModule,
                                                    bool ModuleImported,
                                                    clang::SrcMgr::CharacteristicKind FileType)
{
    check->VisitInclusionDirective(HashLoc,
                                   IncludeTok,
                                   FileName,
                                   IsAngled,
                                   FilenameRange,
                                   File,
                                   SearchPath,
                                   RelativePath,
                                   SuggestedModule,
                                   ModuleImported,
                                   FileType);
}

CheckBase::CheckBase(const std::string &name, const ClazyContext *context, Options options)
    : m_sm(context->sm)
    , m_name(name)
    , m_context(context)
    , m_astContext(context->astContext)
    , m_preprocessorCallbacks(new ClazyPreprocessorCallbacks(this))
    , m_options(options)
    , m_tag(" [-Wclazy-" + m_name + ']')
{
}

CheckBase::~CheckBase()
{
}

void CheckBase::VisitStmt(Stmt *)
{
    // Overridden in derived classes
}

void CheckBase::VisitDecl(Decl *)
{
    // Overridden in derived classes
}

void CheckBase::VisitMacroExpands(const Token &, const SourceRange &, const clang::MacroInfo *)
{
    // Overridden in derived classes
}

void CheckBase::VisitMacroDefined(const Token &)
{
    // Overridden in derived classes
}

void CheckBase::VisitDefined(const Token &, const SourceRange &)
{
    // Overridden in derived classes
}

void CheckBase::VisitIfdef(clang::SourceLocation, const clang::Token &)
{
    // Overridden in derived classes
}

void CheckBase::VisitIfndef(SourceLocation, const Token &)
{
    // Overridden in derived classes
}

void CheckBase::VisitIf(SourceLocation, SourceRange, clang::PPCallbacks::ConditionValueKind)
{
    // Overridden in derived classes
}

void CheckBase::VisitElif(SourceLocation, SourceRange, clang::PPCallbacks::ConditionValueKind, SourceLocation)
{
    // Overridden in derived classes
}

void CheckBase::VisitElse(SourceLocation, SourceLocation)
{
    // Overridden in derived classes
}

void CheckBase::VisitEndif(SourceLocation, SourceLocation)
{
    // Overridden in derived classes
}

void CheckBase::VisitInclusionDirective(clang::SourceLocation,
                                        const clang::Token &,
                                        llvm::StringRef,
                                        bool,
                                        clang::CharSourceRange,
                                        clazy::OptionalFileEntryRef,
                                        llvm::StringRef,
                                        llvm::StringRef,
                                        const clang::Module *,
                                        bool ModuleImported,
                                        clang::SrcMgr::CharacteristicKind)
{
    // Overridden in derived classes
}

void CheckBase::enablePreProcessorCallbacks()
{
    m_context->m_pp.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
}

bool CheckBase::shouldIgnoreFile(SourceLocation loc) const
{
    if (m_filesToIgnore.empty()) {
        return false;
    }

    if (!loc.isValid()) {
        return true;
    }

    std::string filename = static_cast<std::string>(sm().getFilename(loc));

    return clazy::any_of(m_filesToIgnore, [filename](const std::string &ignored) {
        return clazy::contains(filename, ignored);
    });
}

void CheckBase::emitWarning(const clang::Decl *d, const std::string &error, bool printWarningTag)
{
    emitWarning(d->getBeginLoc(), error, printWarningTag);
}

void CheckBase::emitWarning(const clang::Stmt *s, const std::string &error, bool printWarningTag)
{
    emitWarning(s->getBeginLoc(), error, printWarningTag);
}

void CheckBase::emitWarning(clang::SourceLocation loc, const std::string &error, bool printWarningTag)
{
    emitWarning(loc, error, {}, printWarningTag);
}

void CheckBase::emitWarning(clang::SourceLocation loc, std::string error, const std::vector<FixItHint> &fixits, bool printWarningTag)
{
    if (m_context->suppressionManager.isSuppressed(m_name, loc, sm(), lo())) {
        return;
    }

    if (m_context->shouldIgnoreFile(loc)) {
        return;
    }

    if (loc.isMacroID()) {
        if (warningAlreadyEmitted(loc)) {
            return; // For warnings in macro arguments we get a warning in each place the argument is used within the expanded macro, so filter all the dups
        }
        m_emittedWarningsInMacro.push_back(loc.getRawEncoding());
    }

    if (printWarningTag) {
        error += m_tag;
    }

    reallyEmitWarning(loc, error, fixits);

    for (const auto &l : m_queuedManualInterventionWarnings) {
        std::string msg("FixIt failed, requires manual intervention: ");
        if (!l.second.empty()) {
            msg += ' ' + l.second;
        }

        reallyEmitWarning(l.first, msg + m_tag, {});
    }

    m_queuedManualInterventionWarnings.clear();
}

void CheckBase::emitInternalError(SourceLocation loc, std::string error)
{
    llvm::errs() << m_tag << ": internal error: " << error << " at " << loc.printToString(sm()) << "\n";
}
void CheckBase::reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const std::vector<FixItHint> &fixits)
{
    auto severity = (m_context->treatAsError(m_name) || (m_context->astContext.getDiagnostics().getWarningsAsErrors() && !m_context->userDisabledWError()))
        ? DiagnosticIDs::Error
        : DiagnosticIDs::Warning;

    m_context->p_warningReporter(name(), loc, severity, error, fixits);
}

void CheckBase::queueManualFixitWarning(clang::SourceLocation loc, const std::string &message)
{
    if (!manualFixitAlreadyQueued(loc)) {
        m_queuedManualInterventionWarnings.push_back({loc, message});
        m_emittedManualFixItsWarningsInMacro.push_back(loc.getRawEncoding());
    }
}

bool CheckBase::warningAlreadyEmitted(SourceLocation loc) const
{
    PresumedLoc ploc = sm().getPresumedLoc(loc);
    for (auto rawLoc : m_emittedWarningsInMacro) {
        SourceLocation l = SourceLocation::getFromRawEncoding(rawLoc);
        PresumedLoc p = sm().getPresumedLoc(l);
        if (Utils::presumedLocationsEqual(p, ploc)) {
            return true;
        }
    }

    return false;
}

bool CheckBase::manualFixitAlreadyQueued(SourceLocation loc) const
{
    PresumedLoc ploc = sm().getPresumedLoc(loc);
    for (auto loc : m_emittedManualFixItsWarningsInMacro) {
        SourceLocation l = SourceLocation::getFromRawEncoding(loc);
        PresumedLoc p = sm().getPresumedLoc(l);
        if (Utils::presumedLocationsEqual(p, ploc)) {
            return true;
        }
    }

    return false;
}

bool CheckBase::isOptionSet(const std::string &optionName) const
{
    const std::string qualifiedName = name() + '-' + optionName;
    return m_context->isOptionSet(qualifiedName);
}

ClazyAstMatcherCallback::ClazyAstMatcherCallback(CheckBase *check)
    : MatchFinder::MatchCallback()
    , m_check(check)
{
}
