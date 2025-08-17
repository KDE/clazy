/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT6_FWD_FIXES_H
#define CLAZY_QT6_FWD_FIXES_H

#include "checkbase.h"

#include <string>

/**
 * Replaces forward declaration with #include <QtCore/qcontainerfwd.h>.
 *
 * Run only in Qt 6 code.
 */
class Qt6FwdFixes : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *decl) override;
    void VisitInclusionDirective(clang::SourceLocation HashLoc,
                                 const clang::Token &IncludeTok,
                                 clang::StringRef FileName,
                                 bool IsAngled,
                                 clang::CharSourceRange FilenameRange,
                                 clazy::OptionalFileEntryRef File,
                                 clang::StringRef SearchPath,
                                 clang::StringRef RelativePath,
                                 const clang::Module *SuggestedModule,
                                 bool ModuleImported,
                                 clang::SrcMgr::CharacteristicKind FileType) override;
    bool m_including_qcontainerfwd = false;
    std::set<clang::StringRef> m_qcontainerfwd_included_in_files;
    std::string m_currentFile;
};

#endif
