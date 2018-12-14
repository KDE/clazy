/*
    This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_SUPPRESSION_MANAGER_H
#define CLAZY_SUPPRESSION_MANAGER_H

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

namespace clang {
class SourceLocation;
class LangOptions;
class SourceManager;
class FileID;
}

class SuppressionManager
{
public:
    typedef unsigned SourceFileID;
    typedef unsigned LineNumber;
    typedef std::string CheckName;
    typedef std::pair<LineNumber, CheckName> LineAndCheckName;

    struct Suppressions {
        bool skipEntireFile = false;

        std::set<CheckName> checksToSkip;
        std::set<LineAndCheckName> checksToSkipByLine;
    };

    SuppressionManager();

    bool isSuppressed(const std::string &checkName, clang::SourceLocation,
                      const clang::SourceManager &, const clang::LangOptions &) const;

private:
    void parseFile(clang::FileID, const clang::SourceManager &, const clang::LangOptions &lo) const;
    SuppressionManager(const SuppressionManager &) = delete;
    SuppressionManager& operator=(const SuppressionManager &) = delete;
    mutable std::unordered_map<SourceFileID, Suppressions> m_processedFileIDs;
};

#endif
