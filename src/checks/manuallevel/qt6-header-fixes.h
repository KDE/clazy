/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

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

#ifndef CLAZY_QT6_HEADER_FIXES_H
#define CLAZY_QT6_HEADER_FIXES_H

#include "checkbase.h"

#include <vector>
#include <string>

class ClazyContext;

namespace clang {
class Stmt;
class FixItHint;
}

/**
 * Replaces wrong headers with correct ones.
 *
 * Run only in Qt 6 code.
 */
class Qt6HeaderFixes
    : public CheckBase
{
public:
    explicit Qt6HeaderFixes(const std::string &name, ClazyContext *context);
    void VisitInclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok, clang::StringRef FileName, bool IsAngled,
                            clang::CharSourceRange FilenameRange, const clang::FileEntry *File, clang::StringRef SearchPath,
                            clang::StringRef RelativePath, const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType) override;

};

#endif
