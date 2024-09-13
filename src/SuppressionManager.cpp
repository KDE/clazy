/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SuppressionManager.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <algorithm>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Token.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <regex>
#include <vector>

using namespace clang;

SuppressionManager::SuppressionManager()
{
}

bool SuppressionManager::isSuppressed(const std::string &checkName,
                                      clang::SourceLocation loc,
                                      const clang::SourceManager &sm,
                                      const clang::LangOptions &lo) const
{
    if (loc.isMacroID()) {
        loc = sm.getExpansionLoc(loc);
    }

    FileID fileID = sm.getFileID(loc);
    if (fileID.isInvalid()) {
        return false;
    }

    auto it = m_processedFileIDs.find(fileID.getHashValue());
    if ((it == m_processedFileIDs.end())) {
        parseFile(fileID, sm, lo);
        it = m_processedFileIDs.find(fileID.getHashValue());
    }

    Suppressions &suppressions = it->second;

    // Case 1: clazy:skip, the whole file is skipped, regardless of which check
    if (suppressions.skipEntireFile) {
        return true;
    }

    // Case 2: clazy:excludeall=foo, the check foo will be ignored for this file
    if (suppressions.checksToSkip.find(checkName) != suppressions.checksToSkip.cend()) {
        return true;
    }

    // Case 3: clazy:exclude=foo, the check foo will be ignored for this file in this line number
    if (loc.isInvalid()) {
        return false;
    }

    const int lineNumber = sm.getSpellingLineNumber(loc);
    if (suppressions.skipNextLine.count(lineNumber) > 0) {
        suppressions.skipNextLine.erase(lineNumber);
        return true;
    }
    bool isLocallySupresses = //
        std::any_of(suppressions.checksSuppressionScope.begin(),
                    suppressions.checksSuppressionScope.end(),
                    [&checkName, &loc](const ScopedSupression &scopedSuppression) {
                        const std::vector<CheckName> checks = scopedSuppression.second;
                        const bool supressesCheck = //
                            checks.end() != std::find_if(checks.begin(), checks.end(), [&checkName](const std::string &name) {
                                return name == checkName;
                            });
                        return supressesCheck && scopedSuppression.first.fullyContains(SourceRange(loc));
                    });
    if (isLocallySupresses) {
        return true;
    }
    if (suppressions.checksToSkipByLine.find(LineAndCheckName(lineNumber, checkName)) != suppressions.checksToSkipByLine.cend())
        return true;

    return false;
}

bool isEmptyOrWhitespace(const std::string &str)
{
    // Check if the string is empty or if all characters are whitespace
    return str.empty() || std::all_of(str.begin(), str.end(), [](unsigned char c) {
               return std::isspace(c);
           });
}

void SuppressionManager::parseFile(FileID id, const SourceManager &sm, const clang::LangOptions &lo) const
{
    const unsigned hash = id.getHashValue();
    auto it = m_processedFileIDs.insert({hash, Suppressions()}).first;
    Suppressions &suppressions = (*it).second;

    bool invalid = false;
    auto buffer = clazy::getBuffer(sm, id, &invalid);
    if (invalid) {
        llvm::errs() << "SuppressionManager::parseFile: Invalid buffer ";
        if (buffer) {
            llvm::errs() << buffer->getBuffer() << "\n";
        }
        return;
    }

    auto lexer = GET_LEXER(id, buffer, sm, lo);
    lexer.SetCommentRetentionState(true);

    Token token;
    std::stack<std::pair<SourceLocation, std::vector<CheckName>>> scopeStack;

    std::stack<SourceLocation> openingBraceStack;
    SourceRange range;
    std::vector<CheckName> rangeChecks;

    while (!lexer.LexFromRawLexer(token)) {
        if (token.getKind() == tok::l_brace) {
            openingBraceStack.push(token.getLocation());

            // if we open a new scope, but have spressions parsed. Write them away, we will put the end in later
            if (!rangeChecks.empty()) {
                suppressions.checksSuppressionScope.insert(suppressions.checksSuppressionScope.end(), {range, rangeChecks});
                rangeChecks = {};
            }
            range = SourceRange();
            range.setBegin(token.getLocation());

        } else if (token.getKind() == tok::r_brace) {
            const SourceLocation openingSourceLocation = openingBraceStack.top();
            openingBraceStack.pop();

            // Find the suppression entry, because we might have had other scopes opened in between
            auto foundIt = std::find_if(suppressions.checksSuppressionScope.begin(),
                                        suppressions.checksSuppressionScope.end(),
                                        [&openingSourceLocation](const auto &entry) {
                                            SourceRange entryRange = entry.first;
                                            return entryRange.getBegin() == openingSourceLocation;
                                        });
            if (foundIt == suppressions.checksSuppressionScope.end()) {
                if (!rangeChecks.empty()) {
                    range.setEnd(token.getLocation());
                    suppressions.checksSuppressionScope.insert(suppressions.checksSuppressionScope.end(), {range, rangeChecks});
                }
            } else {
                (*foundIt).first.setEnd(token.getLocation());
            }

            range = {};
            rangeChecks = {};
        } else if (token.getKind() == tok::comment) {
            token.getLocation().dump(sm);
            std::string comment = Lexer::getSpelling(token, sm, lo);

            if (clazy::contains(comment, "clazy:skip")) {
                suppressions.skipEntireFile = true;
                return;
            }

            if (clazy::contains(comment, "NOLINTNEXTLINE")) {
                bool invalid = false;
                const int nextLineNumber = sm.getSpellingLineNumber(token.getLocation(), &invalid) + 1;
                if (invalid) {
                    llvm::errs() << "SuppressionManager::parseFile: Invalid line number for token location where NOLINTNEXTLINE was found\n";
                    continue;
                }

                suppressions.skipNextLine.insert(nextLineNumber);
            }

            static std::regex rx(R"(clazy:excludeall=(.*?)(\s|$))");
            std::smatch match;
            if (regex_search(comment, match, rx) && match.size() > 1) {
                std::vector<std::string> checks = clazy::splitString(match[1], ',');
                if (range.getBegin().isValid()) {
                    rangeChecks.insert(rangeChecks.begin(), checks.cbegin(), checks.cend());
                } else {
                    suppressions.checksToSkip.insert(checks.cbegin(), checks.cend());
                }
            }

            const int lineNumber = sm.getSpellingLineNumber(token.getLocation());
            if (lineNumber < 0) {
                llvm::errs() << "SuppressionManager::parseFile: Invalid line number " << lineNumber << "\n";
                continue;
            }

            static std::regex rx2(R"(clazy:exclude=(.*?)(\s|$))");
            if (regex_search(comment, match, rx2) && match.size() > 1) {
                // check
                SourceLocation tokenLocation = token.getLocation();
                unsigned lineNum = sm.getSpellingLineNumber(tokenLocation);
                SourceLocation lineStartLoc = sm.translateLineCol(sm.getFileID(tokenLocation), lineNum, 1);
                const char *lineStart = sm.getCharacterData(lineStartLoc);
                const char *bufferEnd = sm.getBufferData(sm.getFileID(tokenLocation)).end();
                const char *lineEnd = lineStart;
                while (lineEnd < bufferEnd && *lineEnd != '\n' && *lineEnd != '\r') {
                    ++lineEnd;
                }

                const std::string prevFileContent = std::string(lineStart, lineEnd).substr(0, sm.getPresumedColumnNumber(token.getLocation()) - 1);
                std::vector<std::string> checks = clazy::splitString(match[1], ',');
                const int checkLineNumber = isEmptyOrWhitespace(prevFileContent) ? lineNumber + 1 : lineNumber;
                for (const std::string &checkName : checks) {
                    suppressions.checksToSkipByLine.insert(LineAndCheckName(checkLineNumber, checkName));
                }
            }
        }
    }
}
