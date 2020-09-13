/*
    This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

// clazy:excludeall=non-pod-global-static

#include "Clazy.h"
#include "ClazyContext.h"

#include "checks.json.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include <iostream>
#include <string>

namespace clang {
class FrontendAction;
}  // namespace clang

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static llvm::cl::OptionCategory s_clazyCategory("clazy options");
static cl::opt<std::string> s_checks("checks", cl::desc("Comma-separated list of clazy checks. Default is level1"),
                                     cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<std::string> s_exportFixes("export-fixes", cl::desc("YAML file to store suggested fixes in. The stored fixes can be applied to the input source code with clang-apply-replacements."),
                                          cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<bool> s_qt4Compat("qt4-compat", cl::desc("Turns off checks not compatible with Qt 4"),
                                 cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<bool> s_onlyQt("only-qt", cl::desc("Won't emit warnings for non-Qt files, or in other words, if -DQT_CORE_LIB is missing."),
                              cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<bool> s_qtDeveloper("qt-developer", cl::desc("For running clazy on Qt itself, optional, but honours specific guidelines"),
                                   cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<bool> s_visitImplicitCode("visit-implicit-code", cl::desc("For visiting implicit code like compiler generated constructors. None of the built-in checks benefit from this, but can be useful for custom checks"),
                                         cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<bool> s_ignoreIncludedFiles("ignore-included-files", cl::desc("Only emit warnings for the current file being compiled and ignore any includes. Useful for performance reasons."),
                                           cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<std::string> s_headerFilter("header-filter", cl::desc(R"(Regular expression matching the names of the
headers to output diagnostics from. Diagnostics
from the main file of each translation unit are
always displayed.)"),
                                           cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<std::string> s_ignoreDirs("ignore-dirs", cl::desc(R"(Regular expression matching the names of the
directories for which diagnostics should never be emitted. Useful for ignoring 3rdparty code.)"),
                                         cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<bool> s_supportedChecks("supported-checks-json", cl::desc("Dump meta information about supported checks in JSON format."),
                                       cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<bool> s_listEnabledChecks("list-checks", cl::desc("List all enabled checks and exit."),
                                         cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<std::string> s_vfsoverlay("vfsoverlay", cl::desc("YAML file to overlay the virtual filesystem described by file over the real file system."),
                                          cl::init(""), cl::cat(s_clazyCategory));

static cl::extrahelp s_commonHelp(CommonOptionsParser::HelpMessage);

class ClazyToolActionFactory
    : public clang::tooling::FrontendActionFactory
{
public:
    ClazyToolActionFactory(std::vector<std::string> paths)
        : FrontendActionFactory()
        , m_paths(std::move(paths))
    {
    }

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<FrontendAction> create() override
#else
    FrontendAction *create() override
#endif
    {
        ClazyContext::ClazyOptions options = ClazyContext::ClazyOption_None;

        if (!s_exportFixes.getValue().empty())
            options |= ClazyContext::ClazyOption_ExportFixes;

        if (s_qt4Compat.getValue())
            options |= ClazyContext::ClazyOption_Qt4Compat;

        if (s_qtDeveloper.getValue())
            options |= ClazyContext::ClazyOption_QtDeveloper;

        if (s_onlyQt.getValue())
            options |= ClazyContext::ClazyOption_OnlyQt;

        if (s_visitImplicitCode.getValue())
            options |= ClazyContext::ClazyOption_VisitImplicitCode;

        if (s_ignoreIncludedFiles.getValue())
            options |= ClazyContext::ClazyOption_IgnoreIncludedFiles;

        // TODO: We need to agregate the fixes with previous run
#if LLVM_VERSION_MAJOR >= 10
        return std::make_unique<ClazyStandaloneASTAction>(s_checks.getValue(), s_headerFilter.getValue(),
                                                          s_ignoreDirs.getValue(), s_exportFixes.getValue(),
                                                          m_paths, options);
#else
        return new ClazyStandaloneASTAction(s_checks.getValue(), s_headerFilter.getValue(),
                                            s_ignoreDirs.getValue(), s_exportFixes.getValue(),
                                            m_paths, options);
#endif
    }
    std::vector<std::string> m_paths;
};

llvm::IntrusiveRefCntPtr<vfs::FileSystem> getVfsFromFile(const std::string &overlayFile, llvm::IntrusiveRefCntPtr<vfs::FileSystem> BaseFS)
{
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer = BaseFS->getBufferForFile(
        overlayFile);
    if (!buffer) {
        llvm::errs() << "Can't load virtual filesystem overlay file '" << overlayFile
                     << "': " << buffer.getError().message() << ".\n";
        return nullptr;
    }

    IntrusiveRefCntPtr<vfs::FileSystem> fs = vfs::getVFSFromYAML(std::move(buffer.get()),
                                                                 /*DiagHandler*/ nullptr,
                                                                 overlayFile);
    if (!fs) {
        llvm::errs() << "Error: invalid virtual filesystem overlay file '" << overlayFile << "'.\n";
        return nullptr;
    }
    return fs;
}

int main(int argc, const char **argv)
{
    CommonOptionsParser optionsParser(argc, argv, s_clazyCategory, cl::ZeroOrMore);
    // llvm::errs() << optionsParser.getSourcePathList().size() << "\n";

    if (s_supportedChecks.getValue()) {
        std::cout << SUPPORTED_CHECKS_JSON_STR;
        return 0;
    }

    if (s_listEnabledChecks.getValue()) {
        std::string checksFromArgs = s_checks.getValue();
        std::vector<std::string> checks = { checksFromArgs.empty() ? "level1" : checksFromArgs };
        const RegisteredCheck::List enabledChecks
                = CheckManager::instance()->requestedChecks(checks, s_qt4Compat.getValue());

        if (!enabledChecks.empty()) {
            llvm::outs() << "Enabled checks:";
            for (const auto &check : enabledChecks) {
                llvm::outs() << "\n    " << check.name;
            }
            llvm::outs() << "\n";
        }

        return 0;
    }

    llvm::IntrusiveRefCntPtr<vfs::OverlayFileSystem> fs(
        new vfs::OverlayFileSystem(vfs::getRealFileSystem()));
    const std::string &overlayFile = s_vfsoverlay.getValue();
    if (!s_vfsoverlay.getValue().empty()) {
        llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer = fs->getBufferForFile(
            overlayFile);
        if (!buffer) {
            llvm::errs() << "Can't load virtual filesystem overlay file '" << overlayFile
                         << "': " << buffer.getError().message() << ".\n";
            return 0;
        }

        IntrusiveRefCntPtr<vfs::FileSystem> vfso = vfs::getVFSFromYAML(std::move(buffer.get()),
                                                                       /*DiagHandler*/ nullptr,
                                                                       overlayFile);
        if (!vfso) {
            llvm::errs() << "Error: invalid virtual filesystem overlay file '" << overlayFile
                         << "'.\n";
            return 0;
        }
        fs->pushOverlay(vfso);
    }

    ClangTool tool(optionsParser.getCompilations(),
                   optionsParser.getSourcePathList(),
                   std::make_shared<PCHContainerOperations>(),
                   fs);

    return tool.run(new ClazyToolActionFactory(optionsParser.getSourcePathList()));
}
