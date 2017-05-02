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

#include "Clazy.h"
#include "ClazyAnchorHeader.h"

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

#include <memory>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static llvm::cl::OptionCategory s_clazyCategory("clazy options");
static cl::opt<std::string> s_checks("checks", cl::desc("Comma-separated list of clazy checks"),
                                     cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<bool> s_noInplaceFixits("no-inplace-fixits", cl::desc("Fixits will be applied to a separate file (for unit-test use only)"),
                                       cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<bool> s_enableAllFixits("enable-all-fixits", cl::desc("Enables all fixits"),
                                       cl::init(""), cl::cat(s_clazyCategory));

static cl::opt<bool> s_qt4Compat("qt4-compat", cl::desc("Turns off checks not compatible with Qt 4"),
                                 cl::init(false), cl::cat(s_clazyCategory));

static cl::extrahelp s_commonHelp(CommonOptionsParser::HelpMessage);

class ClazyToolAction : public clang::tooling::FrontendActionFactory
{
public:
    ClazyToolAction()
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

        return new ClazyStandaloneASTAction(s_checks.getValue(), options);
    }
};

int main(int argc, const char **argv)
{
    CommonOptionsParser optionsParser(argc, argv, s_clazyCategory);
    ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    return tool.run(new ClazyToolAction());
}
