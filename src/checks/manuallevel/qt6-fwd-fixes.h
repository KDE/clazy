/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT6_FWD_FIXES_H
#define CLAZY_QT6_FWD_FIXES_H

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
 * Replaces forward declaration with #include <QtCore/qcontainerfwd.h>.
 *
 * Run only in Qt 6 code.
 */
class Qt6FwdFixes : public CheckBase
{
public:
    explicit Qt6FwdFixes(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
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
    bool m_including_qcontainerfwd = false;
    std::set<clang::StringRef> m_qcontainerfwd_included_in_files;
    std::string m_currentFile;
};

#endif
