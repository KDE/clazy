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

#include "checkbase.h"
#include "ClazyContext.h"
#include "SourceCompatibilityHelpers.h"
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

#include <vector>
#include <memory>

namespace clang {
class MacroArgs;
class Token;
}  // namespace clang

using namespace clang;
using namespace clang::ast_matchers;
using namespace std;

ClazyPreprocessorCallbacks::ClazyPreprocessorCallbacks(CheckBase *check)
    : check(check)
{
}

void ClazyPreprocessorCallbacks::MacroExpands(const Token &macroNameTok, const MacroDefinition &md,
                                              SourceRange range, const MacroArgs *)
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

void ClazyPreprocessorCallbacks::InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok, clang::StringRef FileName, bool IsAngled,
                                                    clang::CharSourceRange FilenameRange, const clang::FileEntry *File, clang::StringRef SearchPath,
                                                    clang::StringRef RelativePath, const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType)
{
    check->VisitInclusionDirective(HashLoc, IncludeTok, FileName, IsAngled, FilenameRange, File, SearchPath, RelativePath, Imported, FileType);
}

CheckBase::CheckBase(const string &name, const ClazyContext *context, Options options)
    : m_sm(context->ci.getSourceManager())
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
    // Overriden in derived classes
}

void CheckBase::VisitDecl(Decl *)
{
    // Overriden in derived classes
}

void CheckBase::VisitMacroExpands(const Token &, const SourceRange &, const clang::MacroInfo *)
{
    // Overriden in derived classes
}

void CheckBase::VisitMacroDefined(const Token &)
{
    // Overriden in derived classes
}

void CheckBase::VisitDefined(const Token &, const SourceRange &)
{
    // Overriden in derived classes
}

void CheckBase::VisitIfdef(clang::SourceLocation, const clang::Token &)
{
    // Overriden in derived classes
}

void CheckBase::VisitIfndef(SourceLocation, const Token &)
{
    // Overriden in derived classes
}

void CheckBase::VisitIf(SourceLocation, SourceRange,  clang::PPCallbacks::ConditionValueKind)
{
    // Overriden in derived classes
}

void CheckBase::VisitElif(SourceLocation, SourceRange,  clang::PPCallbacks::ConditionValueKind, SourceLocation)
{
    // Overriden in derived classes
}

void CheckBase::VisitElse(SourceLocation, SourceLocation)
{
    // Overriden in derived classes
}

void CheckBase::VisitEndif(SourceLocation, SourceLocation)
{
    // Overriden in derived classes
}

void CheckBase::VisitInclusionDirective(clang::SourceLocation , const clang::Token &, clang::StringRef , bool ,
                        clang::CharSourceRange , const clang::FileEntry *, clang::StringRef ,
                        clang::StringRef , const clang::Module *, clang::SrcMgr::CharacteristicKind )
{
    // Overriden in derived classes
}

void CheckBase::enablePreProcessorCallbacks()
{
    Preprocessor &pi = m_context->ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
}

bool CheckBase::shouldIgnoreFile(SourceLocation loc) const
{
    if (m_filesToIgnore.empty())
        return false;

    if (!loc.isValid())
        return true;

    string filename = static_cast<string>(sm().getFilename(loc));

    return clazy::any_of(m_filesToIgnore, [filename](const std::string &ignored) {
        return clazy::contains(filename, ignored);
    });
}

void CheckBase::emitWarning(const clang::Decl *d, const std::string &error, bool printWarningTag)
{
    emitWarning(clazy::getLocStart(d), error, printWarningTag);
}

void CheckBase::emitWarning(const clang::Stmt *s, const std::string &error, bool printWarningTag)
{
    emitWarning(clazy::getLocStart(s), error, printWarningTag);
}

void CheckBase::emitWarning(clang::SourceLocation loc, const std::string &error, bool printWarningTag)
{
    emitWarning(loc, error, {}, printWarningTag);
}

void CheckBase::emitWarning(clang::SourceLocation loc, std::string error,
                            const vector<FixItHint> &fixits, bool printWarningTag)
{
    if (m_context->suppressionManager.isSuppressed(m_name, loc, sm(), lo()))
        return;

    if (m_context->shouldIgnoreFile(loc))
        return;

    if (loc.isMacroID()) {
        if (warningAlreadyEmitted(loc))
            return; // For warnings in macro arguments we get a warning in each place the argument is used within the expanded macro, so filter all the dups
        m_emittedWarningsInMacro.push_back(loc.getRawEncoding());
    }

    if (printWarningTag)
        error += m_tag;

    reallyEmitWarning(loc, error, fixits);

    for (const auto& l : m_queuedManualInterventionWarnings) {
        string msg = string("FixIt failed, requires manual intervention: ");
        if (!l.second.empty())
            msg += ' ' + l.second;

        reallyEmitWarning(l.first, msg + m_tag, {});
    }

    m_queuedManualInterventionWarnings.clear();
}

void CheckBase::emitInternalError(SourceLocation loc, string error)
{
    llvm::errs() << m_tag << ": internal error: " << error
                 << " at " << loc.printToString(sm()) << "\n";
}

void CheckBase::reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const vector<FixItHint> &fixits)
{
    FullSourceLoc full(loc, sm());
    auto &engine = m_context->ci.getDiagnostics();
    auto severity = (m_context->treatAsError(m_name) || (engine.getWarningsAsErrors() && !m_context->userDisabledWError()))
            ? DiagnosticIDs::Error
            : DiagnosticIDs::Warning;
    unsigned id = engine.getDiagnosticIDs()->getCustomDiagID(severity, error.c_str());
    DiagnosticBuilder B = engine.Report(full, id);
    for (const FixItHint& fixit : fixits) {
        if (!fixit.isNull())
            B.AddFixItHint(fixit);
    }
}

void CheckBase::queueManualFixitWarning(clang::SourceLocation loc, const string &message)
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
        if (Utils::presumedLocationsEqual(p, ploc))
            return true;
    }

    return false;
}

bool CheckBase::manualFixitAlreadyQueued(SourceLocation loc) const
{
    PresumedLoc ploc = sm().getPresumedLoc(loc);
    for (auto loc : m_emittedManualFixItsWarningsInMacro) {
        SourceLocation l = SourceLocation::getFromRawEncoding(loc);
        PresumedLoc p = sm().getPresumedLoc(l);
        if (Utils::presumedLocationsEqual(p, ploc))
            return true;
    }

    return false;
}

bool CheckBase::isOptionSet(const std::string &optionName) const
{
    const string qualifiedName = name() + '-' + optionName;
    return m_context->isOptionSet(qualifiedName);
}

ClazyAstMatcherCallback::ClazyAstMatcherCallback(CheckBase *check)
    : MatchFinder::MatchCallback()
    , m_check(check)
{
}
