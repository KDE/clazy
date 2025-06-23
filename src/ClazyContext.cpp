/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ClazyContext.h"
#include "AccessSpecifierManager.h"
#include "FixItExporter.h"
#include "PreProcessorVisitor.h"
#include "checkmanager.h"

#include <clang/AST/ParentMap.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>
#include <llvm/Support/Regex.h>

#include <stdlib.h>

using namespace clang;

ClazyContext::ClazyContext(clang::ASTContext *context,
                           clang::SourceManager &manager,
                           const clang::LangOptions &lo,
                           const clang::PreprocessorOptions &pp,
                           const std::string &headerFilter,
                           const std::string &ignoreDirs,
                           std::string exportFixesFilename,
                           const std::vector<std::string> &translationUnitPaths,
                           ClazyOptions opts,
                           std::optional<WarningReporter> warningReporter,
                           bool isClangTidy)
    : astContext(context)
    , sm(manager)
    , lo(lo)
    , m_noWerror(getenv("CLAZY_NO_WERROR") != nullptr) // Allows user to make clazy ignore -Werror
    , m_checksPromotedToErrors(CheckManager::instance()->checksAsErrors())
    , options(opts)
    , extraOptions(clazy::splitString(getenv("CLAZY_EXTRA_OPTIONS"), ','))
    , m_translationUnitPaths(translationUnitPaths)
    , m_pp(pp)
    , p_warningReporter(warningReporter.value_or([this](std::string,
                                                        const clang::SourceLocation &loc,
                                                        clang::DiagnosticIDs::Level level,
                                                        std::string error,
                                                        const std::vector<clang::FixItHint> &fixits) {
        auto &engine = astContext->getDiagnostics();
        unsigned id = engine.getDiagnosticIDs()->getCustomDiagID(level, error.c_str());
        DiagnosticBuilder B = engine.Report(loc, id);
        for (const FixItHint &fixit : fixits) {
            if (!fixit.isNull()) {
                B.AddFixItHint(fixit);
            }
        }
    }))
    , m_isClangTidy(isClangTidy)
{
    if (!headerFilter.empty()) {
        headerFilterRegex = std::unique_ptr<llvm::Regex>(new llvm::Regex(headerFilter));
    }

    if (!ignoreDirs.empty()) {
        ignoreDirsRegex = std::unique_ptr<llvm::Regex>(new llvm::Regex(ignoreDirs));
    }

    if (exportFixesEnabled() && context) {
        if (exportFixesFilename.empty()) {
            // Only clazy-standalone sets the filename by argument.
            // clazy plugin sets it automatically here:
            const auto fileEntry = sm.getFileEntryRefForID(sm.getMainFileID());
            exportFixesFilename = fileEntry->getName().str() + ".clazy.yaml";
        }

        const bool isClazyStandalone = !translationUnitPaths.empty();
        exporter = new FixItExporter(context->getDiagnostics(), sm, context->getLangOpts(), exportFixesFilename, isClazyStandalone);
    }
}

ClazyContext::~ClazyContext()
{
    // delete preprocessorVisitor; // we don't own it
    delete accessSpecifierManager;
    delete parentMap;

    static unsigned long count = 0;
    count++;

    if (exporter) {
        // With clazy-standalone we use the same YAML file for all translation-units, so only
        // write out the last one. With clazy-plugin there's a YAML file per translation unit.
        const bool isClazyPlugin = m_translationUnitPaths.empty();
        const bool isLast = count == m_translationUnitPaths.size();
        if (isLast || isClazyPlugin) {
            exporter->Export();
        }
        delete exporter;
    }

    preprocessorVisitor = nullptr;
    accessSpecifierManager = nullptr;
    parentMap = nullptr;
}

void ClazyContext::registerPreprocessorCallbacks(clang::Preprocessor &pp)
{
    if (!usingPreCompiledHeaders()) {
        accessSpecifierManager = new AccessSpecifierManager(pp, exportFixesEnabled());
        preprocessorVisitor = new PreProcessorVisitor(pp);
    }
}

bool ClazyContext::isQt() const
{
    static const bool s_isQt = [this] {
        for (const auto &s : m_pp.Macros) {
            if (s.first == "QT_CORE_LIB") {
                return true;
            }
        }
        return false;
    }();

    return s_isQt;
}
bool ClazyContext::usingPreCompiledHeaders() const
{
    return !m_pp.ImplicitPCHInclude.empty();
}
