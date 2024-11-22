/*
    SPDX-FileCopyrightText: 2023 Johnny Jazeix <jazeix@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_NO_MODULE_INCLUDE_H
#define CLAZY_NO_MODULE_INCLUDE_H

#include "checkbase.h"

#include <string>
#include <vector>

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
#if LLVM_VERSION_MAJOR >= 19
                                 const clang::Module *SuggestedModule,
                                 bool ModuleImported,
#else
                                 const clang::Module *Imported,
#endif
                                 clang::SrcMgr::CharacteristicKind FileType) override;

private:
    const std::vector<std::string> m_modulesList;
};

#endif
