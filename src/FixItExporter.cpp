/*
    SPDX-FileCopyrightText: 2019 Christian Gagneraud <chgans@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "FixItExporter.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>
#include <clang/Tooling/DiagnosticsYaml.h>

// #define DEBUG_FIX_IT_EXPORTER

using namespace clang;

static clang::tooling::TranslationUnitDiagnostics &getTuDiag()
{
    static clang::tooling::TranslationUnitDiagnostics s_tudiag;
    return s_tudiag;
}

FixItExporter::FixItExporter(DiagnosticsEngine &DiagEngine,
                             SourceManager &SourceMgr,
                             const LangOptions &LangOpts,
                             const std::string &exportFixes,
                             bool isClazyStandalone)
    : DiagEngine(DiagEngine)
    , SourceMgr(SourceMgr)
    , LangOpts(LangOpts)
    , exportFixes(exportFixes)
{
    if (!isClazyStandalone) {
        // When using clazy as plugin each translation unit fixes goes to a separate YAML file
        getTuDiag().Diagnostics.clear();
    }

    Owner = DiagEngine.takeClient();
    Client = DiagEngine.getClient();
    DiagEngine.setClient(this, false);
}

FixItExporter::~FixItExporter()
{
    if (Client) {
        DiagEngine.setClient(Client, Owner.release() != nullptr);
    }
}

void FixItExporter::BeginSourceFile(const LangOptions &LangOpts, const Preprocessor *PP)
{
    if (Client) {
        Client->BeginSourceFile(LangOpts, PP);
    }

    const auto id = SourceMgr.getMainFileID();
    const auto entry = SourceMgr.getFileEntryRefForID(id);
    getTuDiag().MainSourceFile = entry->getName().str();
}

bool FixItExporter::IncludeInDiagnosticCounts() const
{
    return Client ? Client->IncludeInDiagnosticCounts() : true;
}

void FixItExporter::EndSourceFile()
{
    if (Client) {
        Client->EndSourceFile();
    }
}

tooling::Diagnostic FixItExporter::ConvertDiagnostic(const Diagnostic &Info)
{
    SmallString<256> TmpMessageText;
    Info.FormatDiagnostic(TmpMessageText);
    // TODO: This returns an empty string: DiagEngine->getDiagnosticIDs()->getWarningOptionForDiag(Info.getID());
    // HACK: capture it at the end of the message: Message text [check-name]

    std::string checkName = static_cast<std::string>(DiagEngine.getDiagnosticIDs()->getWarningOptionForDiag(Info.getID()));
    std::string messageText;

    if (checkName.empty()) {
        // Non-built-in clang warnings have the [checkName] in the message
        messageText = TmpMessageText.slice(0, TmpMessageText.find_last_of('[') - 1).str();

        checkName = TmpMessageText.slice(TmpMessageText.find_last_of('[') + 3, TmpMessageText.find_last_of(']')).str();
    } else {
        messageText = TmpMessageText.c_str();
    }

    llvm::StringRef CurrentBuildDir; // Not needed?

    tooling::Diagnostic ToolingDiag(checkName, tooling::Diagnostic::Warning, CurrentBuildDir);
    // FIXME: Sometimes the file path is an empty string.
    if (Info.getLocation().isMacroID()) {
        auto MacroLoc = SourceMgr.getFileLoc(Info.getLocation());
        ToolingDiag.Message = tooling::DiagnosticMessage(messageText, SourceMgr, MacroLoc);
    } else {
        ToolingDiag.Message = tooling::DiagnosticMessage(messageText, SourceMgr, Info.getLocation());
    }
    return ToolingDiag;
}

tooling::Replacement FixItExporter::ConvertFixIt(const FixItHint &Hint)
{
    // TODO: Proper handling of macros
    // https://stackoverflow.com/questions/24062989/clang-fails-replacing-a-statement-if-it-contains-a-macro
    tooling::Replacement Replacement;
    if (Hint.CodeToInsert.empty()) {
        if (Hint.InsertFromRange.isValid()) {
            clang::SourceLocation b(Hint.InsertFromRange.getBegin());
            clang::SourceLocation _e(Hint.InsertFromRange.getEnd());
            if (b.isMacroID()) {
                b = SourceMgr.getSpellingLoc(b);
            }
            if (_e.isMacroID()) {
                _e = SourceMgr.getSpellingLoc(_e);
            }
            clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, SourceMgr, LangOpts));
            StringRef Text(SourceMgr.getCharacterData(b), SourceMgr.getCharacterData(e) - SourceMgr.getCharacterData(b));
            return {SourceMgr, Hint.RemoveRange, Text};
        }
        return {SourceMgr, Hint.RemoveRange, ""};
    }
    return {SourceMgr, Hint.RemoveRange, Hint.CodeToInsert};
}

void FixItExporter::HandleDiagnostic(DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info)
{
    // Default implementation (Warnings/errors count).
    DiagnosticConsumer::HandleDiagnostic(DiagLevel, Info);

    // Let original client do it's handling
    if (Client) {
        Client->HandleDiagnostic(DiagLevel, Info);
    }

    // Convert and record warning diagnostics and their notes
    if (DiagLevel == DiagnosticsEngine::Warning) {
        auto ToolingDiag = ConvertDiagnostic(Info);
        for (unsigned Idx = 0, Last = Info.getNumFixItHints(); Idx < Last; ++Idx) {
            const FixItHint &Hint = Info.getFixItHint(Idx);
            const auto replacement = ConvertFixIt(Hint);
#ifdef DEBUG_FIX_IT_EXPORTER
            const auto FileName = SourceMgr.getFilename(Info.getLocation());
            llvm::errs() << "Handling Fixit #" << Idx << " for " << FileName.str() << "\n";
            llvm::errs() << "F: " << Hint.RemoveRange.getBegin().printToString(SourceMgr) << ":" << Hint.RemoveRange.getEnd().printToString(SourceMgr) << " "
                         << Hint.InsertFromRange.getBegin().printToString(SourceMgr) << ":" << Hint.InsertFromRange.getEnd().printToString(SourceMgr) << " "
                         << Hint.BeforePreviousInsertions << " " << Hint.CodeToInsert << "\n";
            llvm::errs() << "R: " << replacement.toString() << "\n";
#endif
            clang::tooling::Replacements &Replacements = ToolingDiag.Message.Fix[replacement.getFilePath()];
            llvm::Error error = Replacements.add(ConvertFixIt(Hint));
            if (error) {
                Diag(Info.getLocation(), diag::note_fixit_failed);
            }
        }
        getTuDiag().Diagnostics.push_back(ToolingDiag);
        m_recordNotes = true;
    }
    // FIXME: We do not receive notes.
    else if (DiagLevel == DiagnosticsEngine::Note && m_recordNotes) {
#ifdef DEBUG_FIX_IT_EXPORTER
        const auto FileName = SourceMgr.getFilename(Info.getLocation());
        llvm::errs() << "Handling Note for " << FileName.str() << "\n";
#endif
        auto diags = getTuDiag().Diagnostics.back();
        auto diag = ConvertDiagnostic(Info);
        diags.Notes.append(1, diag.Message);
    } else {
        m_recordNotes = false;
    }
}

void FixItExporter::Export()
{
    auto tuDiag = getTuDiag();
    if (!tuDiag.Diagnostics.empty()) {
        std::error_code EC;
        llvm::raw_fd_ostream OS(exportFixes, EC, llvm::sys::fs::OF_None);
        llvm::yaml::Output YAML(OS);
        YAML << getTuDiag();
    }
}

void FixItExporter::Diag(SourceLocation Loc, unsigned DiagID)
{
    // When producing this diagnostic, we temporarily bypass ourselves,
    // clear out any current diagnostic, and let the downstream client
    // format the diagnostic.
    DiagEngine.setClient(Client, false);
#if LLVM_VERSION_MAJOR < 20
    DiagEngine.Clear();
#endif
    DiagEngine.Report(Loc, DiagID);
    DiagEngine.setClient(this, false);
}
