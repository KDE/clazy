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

#ifndef CLAZY_PREPROCESSOR_VISITOR_H
#define CLAZY_PREPROCESSOR_VISITOR_H

// Each check can visit the preprocessor, but doing things per-check can be
// time consuming and we might want to do them only once.
// For example, getting the Qt version can be done once and the result shared
// with all checks

#include "checkbase.h"

#include <clang/Lex/PPCallbacks.h>
#include <llvm/ADT/StringRef.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace clang {
class CompilerInstance;
class SourceManager;
class SourceRange;
class Token;
class MacroDefinition;
class MacroArgs;
class SourceLocation;
}

using uint = unsigned;

class PreProcessorVisitor
    : public clang::PPCallbacks
{
    PreProcessorVisitor(const PreProcessorVisitor &) = delete;
public:
    explicit PreProcessorVisitor(const clang::CompilerInstance &ci);

    // Returns for example 050601 (Qt 5.6.1), or -1 if we don't know the version
    int qtVersion() const { return m_qtVersion; }

    bool isBetweenQtNamespaceMacros(clang::SourceLocation loc);

    // Returns true if QT_NO_KEYWORDS is defined
    bool isQT_NO_KEYWORDS() const { return m_isQtNoKeywords; }

    bool hasInclude(const std::string& fileName, bool IsAngled) const;
    clang::SourceLocation endOfIncludeSection() const;

protected:
    void MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &,
                      clang::SourceRange range, const clang::MacroArgs *) override;
    void InclusionDirective (clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
                             clang::StringRef FileName, bool IsAngled, clang::CharSourceRange FilenameRange,
                             const clang::FileEntry *File, clang::StringRef SearchPath, clang::StringRef RelativePath,
                             const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType) override;
private:
    std::string getTokenSpelling(const clang::MacroDefinition &) const;
    void updateQtVersion();
    void handleQtNamespaceMacro(clang::SourceLocation loc, clang::StringRef name);

    const clang::CompilerInstance &m_ci;
    int m_qtMajorVersion  = -1;
    int m_qtMinorVersion  = -1;
    int m_qtPatchVersion = -1;
    int m_qtVersion = -1;
    bool m_isQtNoKeywords = false;

    // Indexed by FileId, has a list of QT_BEGIN_NAMESPACE/QT_END_NAMESPACE location
    std::unordered_map<uint, std::vector<clang::SourceRange>> m_q_namespace_macro_locations;
    const clang::SourceManager &m_sm;

    struct IncludeInfo {
        clang::StringRef fileName;
        bool IsAngled;
        clang::CharSourceRange filenameRange;
    };

    std::vector<IncludeInfo> m_includeInfo;
};

#endif
