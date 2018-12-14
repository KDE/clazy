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

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

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

static cl::opt<bool> s_noInplaceFixits("no-inplace-fixits", cl::desc("Fixits will be applied to a separate file (for unit-test use only)"),
                                       cl::init(false), cl::cat(s_clazyCategory));

static cl::opt<bool> s_enableAllFixits("enable-all-fixits", cl::desc("Enables all fixits"),
                                       cl::init(false), cl::cat(s_clazyCategory));

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

static cl::extrahelp s_commonHelp(CommonOptionsParser::HelpMessage);

class ClazyToolActionFactory : public clang::tooling::FrontendActionFactory
{
public:
    ClazyToolActionFactory()
        : FrontendActionFactory() {}

    FrontendAction *create() override
    {
        ClazyContext::ClazyOptions options = ClazyContext::ClazyOption_None;
        if (s_noInplaceFixits.getValue())
            options |= ClazyContext::ClazyOption_NoFixitsInplace;

        if (s_enableAllFixits.getValue())
            options |= ClazyContext::ClazyOption_AllFixitsEnabled;

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

        return new ClazyStandaloneASTAction(s_checks.getValue(), s_headerFilter.getValue(), s_ignoreDirs.getValue(), options);
    }
};

int main(int argc, const char **argv)
{
    CommonOptionsParser optionsParser(argc, argv, s_clazyCategory);
    ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    return tool.run(new ClazyToolActionFactory());
}
