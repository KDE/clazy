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

#ifndef CLAZY_FIX_IT_EXPORTER_H
#define CLAZY_FIX_IT_EXPORTER_H

#include <clang/Basic/Diagnostic.h>
#include <clang/Tooling/Core/Diagnostic.h>

namespace clang {
class FixItOptions;
}

class FixItExporter
    : public clang::DiagnosticConsumer
{
public:
    explicit FixItExporter(clang::DiagnosticsEngine &DiagEngine, clang::SourceManager &SourceMgr,
                           const clang::LangOptions &LangOpts, const std::string &exportFixes,
                           bool isClazyStandalone);

    ~FixItExporter() override;

    bool IncludeInDiagnosticCounts() const override;

    void BeginSourceFile(const clang::LangOptions &LangOpts,
                         const clang::Preprocessor *PP = nullptr) override;

    void EndSourceFile() override;

    void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                          const clang::Diagnostic &Info) override;

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
