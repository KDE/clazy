/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_SUPPRESSION_MANAGER_H
#define CLAZY_SUPPRESSION_MANAGER_H

#include <llvm/Support/Regex.h>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace llvm
{
class Regex;
}

namespace clang
{
class SourceLocation;
class LangOptions;
class SourceManager;
class FileID;
}

class SuppressionManager
{
public:
    using SourceFileID = unsigned int;
    using LineNumber = unsigned int;
    using CheckName = std::string;
    using LineAndCheckName = std::pair<LineNumber, CheckName>;
    struct CheckBeginAndEnd {
        std::vector<llvm::Regex> checksToSkipWildcard;
        LineNumber begin;
        LineNumber end;
    };

    struct Suppressions {
        bool skipEntireFile = false;
        std::set<unsigned> skipNextLine;
        std::set<CheckName> checksToSkip;
        std::set<LineAndCheckName> checksToSkipByLine;
        std::unordered_map<LineNumber, llvm::Regex> checkWildcardsToSkipByLine;
        std::vector<CheckBeginAndEnd> checkWildcardsRanges;
    };

    SuppressionManager();

    bool isSuppressed(const std::string &checkName, clang::SourceLocation, const clang::SourceManager &, const clang::LangOptions &) const;

private:
    void parseFile(clang::FileID, const clang::SourceManager &, const clang::LangOptions &lo) const;
    SuppressionManager(const SuppressionManager &) = delete;
    SuppressionManager &operator=(const SuppressionManager &) = delete;
    mutable std::unordered_map<SourceFileID, Suppressions> m_processedFileIDs;
};

#endif
