/*
  This file is part of the clazy static checker.

    Copyright (C) 2019 Christian Gagneraud <chgans@gmail.com>

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

#include "FixItExporter.h"

#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Tooling/DiagnosticsYaml.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>

#define DEBUG_FIX_IT_EXPORTER

#ifdef DEBUG_FIX_IT_EXPORTER
# include <iostream>
#endif

using namespace clang;

FixItExporter::FixItExporter(DiagnosticsEngine &DiagEngine, SourceManager &SourceMgr,
                             const LangOptions &LangOpts, FixItOptions *FixItOpts)
    : DiagEngine(DiagEngine), SourceMgr(SourceMgr), LangOpts(LangOpts)
    , FixItOpts(FixItOpts)
{
    Owner = DiagEngine.takeClient();
    Client = DiagEngine.getClient();
    DiagEngine.setClient(this, false);
}

FixItExporter::~FixItExporter()
{
    DiagEngine.setClient(Client, Owner.release() != nullptr);
}

void FixItExporter::BeginSourceFile(const LangOptions &LangOpts, const Preprocessor *PP)
{
    if (Client)
        Client->BeginSourceFile(LangOpts, PP);

    const auto id = SourceMgr.getMainFileID();
    const auto entry = SourceMgr.getFileEntryForID(id);
    TUDiag.MainSourceFile = entry->getName();
}

bool FixItExporter::IncludeInDiagnosticCounts() const
{
    return Client ? Client->IncludeInDiagnosticCounts() : true;
}

void FixItExporter::EndSourceFile()
{
    if (Client)
        Client->EndSourceFile();
}

tooling::Diagnostic FixItExporter::ConvertDiagnostic(const Diagnostic &Info)
{
    SmallString<100> TmpMessageText;
    Info.FormatDiagnostic(TmpMessageText);
    const auto MessageText = TmpMessageText.slice(0, TmpMessageText.find_last_of('[') - 1).str();
    const auto CheckName = TmpMessageText.slice(TmpMessageText.find_last_of('[') + 3,
                                                TmpMessageText.find_last_of(']')).str();
    llvm::StringRef CurrentBuildDir; // Not needed?

    tooling::Diagnostic ToolingDiag(CheckName,
                                    tooling::Diagnostic::Warning,
                                    CurrentBuildDir);
    ToolingDiag.Message = tooling::DiagnosticMessage(MessageText, SourceMgr, Info.getLocation());
    return ToolingDiag;
}

tooling::Replacement FixItExporter::ConvertFixIt(const FixItHint &Hint)
{
    tooling::Replacement Replacement;
    if (Hint.CodeToInsert.empty()) {
        if (Hint.InsertFromRange.isValid()) {
            clang::SourceLocation b(Hint.InsertFromRange.getBegin()), _e(Hint.InsertFromRange.getEnd());
            if (b.isMacroID())
                b = SourceMgr.getSpellingLoc(b);
            if (_e.isMacroID())
                _e = SourceMgr.getSpellingLoc(_e);
            clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, SourceMgr, LangOpts));
            StringRef Text(SourceMgr.getCharacterData(b),
                           SourceMgr.getCharacterData(e)-SourceMgr.getCharacterData(b));
            return tooling::Replacement(SourceMgr, Hint.RemoveRange, Text);
        }
        return tooling::Replacement(SourceMgr, Hint.RemoveRange, "");
    }
    return tooling::Replacement(SourceMgr, Hint.RemoveRange, Hint.CodeToInsert);
}

void FixItExporter::HandleDiagnostic(DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info)
{
    // Default implementation (Warnings/errors count).
    DiagnosticConsumer::HandleDiagnostic(DiagLevel, Info);

    // Let origianl client do it's handling
    if (Client)
        Client->HandleDiagnostic(DiagLevel, Info);

    // We only deal with warnings
    if (DiagLevel != DiagnosticsEngine::Warning)
        return;

    // Convert and record this diagnostic
    auto ToolingDiag = ConvertDiagnostic(Info);
    for (unsigned Idx = 0, Last = Info.getNumFixItHints();
         Idx < Last; ++Idx) {
        const FixItHint &Hint = Info.getFixItHint(Idx);
        const auto replacement = ConvertFixIt(Hint);
#ifdef DEBUG_FIX_IT_EXPORTER
        const auto FileName = SourceMgr.getFilename(Info.getLocation());
        std::cerr << "Handling Fixit #" << Idx << " for " << FileName.str() << std::endl;
        std::cerr << "F: "
                  << Hint.RemoveRange.getBegin().printToString(SourceMgr) << ":"
                  << Hint.RemoveRange.getEnd().printToString(SourceMgr) << " "
                  << Hint.InsertFromRange.getBegin().printToString(SourceMgr) << ":"
                  << Hint.InsertFromRange.getEnd().printToString(SourceMgr) << " "
                  << Hint.BeforePreviousInsertions << " "
                  << Hint.CodeToInsert << std::endl;
        std::cerr << "R: " << replacement.toString() << std::endl;
#endif
        auto &Replacements = ToolingDiag.Fix[replacement.getFilePath()];
        auto error = Replacements.add(ConvertFixIt(Hint));
        if (error) {
            Diag(Info.getLocation(), diag::note_fixit_failed);
        }
    }
    TUDiag.Diagnostics.push_back(ToolingDiag);
}

void FixItExporter::Export()
{
    std::error_code EC;
    llvm::raw_fd_ostream OS(TUDiag.MainSourceFile + ".yaml", EC, llvm::sys::fs::F_None);
    llvm::yaml::Output YAML(OS);
    YAML << TUDiag;
}

void FixItExporter::Diag(SourceLocation Loc, unsigned DiagID)
{
    // When producing this diagnostic, we temporarily bypass ourselves,
    // clear out any current diagnostic, and let the downstream client
    // format the diagnostic.
    DiagEngine.setClient(Client, false);
    DiagEngine.Clear();
    DiagEngine.Report(Loc, DiagID);
    DiagEngine.setClient(this, false);
}
