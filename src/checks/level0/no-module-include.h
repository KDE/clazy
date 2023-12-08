/*
  This file is part of the clazy static checker.

    Copyright (C) 2023 Johnny Jazeix <jazeix@gmail.com>

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

#ifndef CLAZY_NO_MODULE_INCLUDE_H
#define CLAZY_NO_MODULE_INCLUDE_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * See README-no-module-include.md for more info.
 */
class NoModuleInclude : public CheckBase
{
public:
    explicit NoModuleInclude(const std::string &name, ClazyContext *context);
    void VisitInclusionDirective(clang::SourceLocation HashLoc,
                                 const clang::Token &IncludeTok,
                                 clang::StringRef FileName,
                                 bool IsAngled,
                                 clang::CharSourceRange FilenameRange,
                                 clazy::OptionalFileEntryRef File,
                                 clang::StringRef SearchPath,
                                 clang::StringRef RelativePath,
                                 const clang::Module *Imported,
                                 clang::SrcMgr::CharacteristicKind FileType) override;

private:
    const std::vector<std::string> m_modulesList;
};

#endif
