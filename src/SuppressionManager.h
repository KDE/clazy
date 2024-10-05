/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_SUPPRESSION_MANAGER_H
#define CLAZY_SUPPRESSION_MANAGER_H

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace clang
{
class SourceLocation;
class SourceRange;
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

    struct Suppressions {
        bool skipEntireFile = false;
        std::set<unsigned> skipNextLine;
        std::set<CheckName> checksToSkip;
        std::set<LineAndCheckName> checksToSkipByLine;
        std::vector<std::pair<clang::SourceRange, std::vector<CheckName>>> checksSuppressionScope;
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
