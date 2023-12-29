/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT6_HEADER_FIXES_H
#define CLAZY_QT6_HEADER_FIXES_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
class Stmt;
class FixItHint;
}

/**
 * Replaces wrong headers with correct ones.
 *
 * Run only in Qt 6 code.
 */
class Qt6HeaderFixes : public CheckBase
{
public:
    explicit Qt6HeaderFixes(const std::string &name, ClazyContext *context);
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
};

#endif
