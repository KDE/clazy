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
#include <llvm/Support/JSON.h>
#include <llvm/Support/Regex.h>

#include <cstdlib>

using namespace clang;

// Parses the -line-filter argument, which uses the same JSON format as clang-tidy, e.g.:
//   [{"name":"file1.cpp","lines":[[1,3],[5,7]]},{"name":"file2.h"}]
// An entry without "lines" matches the whole file.

static std::vector<ClazyContext::LineFilterEntry> parseLineFilter(const std::string &lineFilter)
{
    std::vector<ClazyContext::LineFilterEntry> result;

    llvm::Expected<llvm::json::Value> parsed = llvm::json::parse(lineFilter);
    if (!parsed) {
        llvm::errs() << "clazy: ignoring invalid -line-filter: " << llvm::toString(parsed.takeError()) << "\n";
        return result;
    }

    const llvm::json::Array *files = parsed->getAsArray();
    if (!files) {
        llvm::errs() << "clazy: ignoring -line-filter, expected as a JSON array\n";
        return result;
    }

    for (const llvm::json::Value &fileValue : *files) {
        const llvm::json::Object *fileObject = fileValue.getAsObject();
        if (!fileObject) {
            continue;
        }

        std::optional<llvm::StringRef> name = fileObject->getString("name");
        if (!name) {
            continue;
        }

        ClazyContext::LineFilterEntry entry;
        entry.fileName = name->str();

        if (const llvm::json::Array *lines = fileObject->getArray("lines")) {
            for (const llvm::json::Value &rangeValue : *lines) {
                const llvm::json::Array *range = rangeValue.getAsArray();
                if (!range || range->size() != 2) {
                    continue;
                }
                std::optional<int64_t> start = (*range)[0].getAsInteger();
                std::optional<int64_t> end = (*range)[1].getAsInteger();
                if (start && end) {
                    entry.lineRanges.push_back({static_cast<unsigned>(*start), static_cast<unsigned>(*end)});
                }
            }
        }

        result.push_back(std::move(entry));
    }
    return result;
}

ClazyContext::ClazyContext(clang::ASTContext *context,
                           clang::SourceManager &manager,
                           const clang::LangOptions &lo,
                           const clang::PreprocessorOptions &pp,
                           const std::string &headerFilter,
                           const std::string &ignoreDirs,
                           const std::string &lineFilter,
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
        headerFilterRegex = std::make_unique<llvm::Regex>(headerFilter);
    }

    if (!ignoreDirs.empty()) {
        ignoreDirsRegex = std::make_unique<llvm::Regex>(ignoreDirs);
    }

    if (!lineFilter.empty()) {
        m_lineFilter = parseLineFilter(lineFilter);
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
std::string ClazyContext::qtNamespace() const
{
    return accessSpecifierManager ? accessSpecifierManager->qtNamespace() : "";
}
bool ClazyContext::usingPreCompiledHeaders() const
{
    return !m_pp.ImplicitPCHInclude.empty();
}

bool ClazyContext::passesLineFilter(clang::SourceLocation loc) const
{
    if (m_lineFilter.empty()) {
        return true;
    }

    const PresumedLoc ploc = sm.getPresumedLoc(loc);
    if (ploc.isInvalid()) {
        return true; // Don't drop diagnostics we can't attribute to a location
    }

    const llvm::StringRef fileName(ploc.getFilename());
    const unsigned line = ploc.getLine();

    for (const LineFilterEntry &entry : m_lineFilter) {
        // clang-tidy matches by filename suffix, which copes with relative paths from a diff.
        if (!fileName.ends_with(entry.fileName)) {
            continue;
        }

        if (entry.lineRanges.empty()) {
            return true; // Whole file is whitelisted
        }

        for (const LineRange &range : entry.lineRanges) {
            if (line >= range.start && line <= range.end) {
                return true;
            }
        }
    }

    return false; // A filter is active and this file/line is not part of it
}
