/*
    SPDX-FileCopyrightText: 2019 Christian Gagneraud <chgans@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_FIX_IT_EXPORTER_H
#define CLAZY_FIX_IT_EXPORTER_H

#include <clang/Basic/Diagnostic.h>
#include <clang/Tooling/Core/Diagnostic.h>

namespace clang
{
class FixItOptions;
}

class FixItExporter : public clang::DiagnosticConsumer
{
public:
    explicit FixItExporter(clang::DiagnosticsEngine &DiagEngine,
                           clang::SourceManager &SourceMgr,
                           const clang::LangOptions &LangOpts,
                           const std::string &exportFixes,
                           bool isClazyStandalone);

    ~FixItExporter() override;

    bool IncludeInDiagnosticCounts() const override;

    void BeginSourceFile(const clang::LangOptions &LangOpts, const clang::Preprocessor *PP = nullptr) override;

    void EndSourceFile() override;

    void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic &Info) override;

    void Export();

    /// Emit a diagnostic via the adapted diagnostic client.
    void Diag(clang::SourceLocation Loc, unsigned DiagID);

private:
    clang::DiagnosticsEngine &DiagEngine;
    clang::SourceManager &SourceMgr;
    const clang::LangOptions &LangOpts;
    const std::string exportFixes;
    DiagnosticConsumer *Client = nullptr;
    std::unique_ptr<DiagnosticConsumer> Owner;
    bool m_recordNotes = false;
    clang::tooling::Diagnostic ConvertDiagnostic(const clang::Diagnostic &Info);
    clang::tooling::Replacement ConvertFixIt(const clang::FixItHint &Hint);
};

#endif // CLAZY_FIX_IT_EXPORTER_H
