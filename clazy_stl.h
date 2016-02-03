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


#ifndef CLAZY_STL_H
#define CLAZY_STL_H

#include <clang/AST/Stmt.h>
#include <algorithm>
#include <sstream>

namespace clazy_std {

// Don't use .begin() or cend(), clang's ranges don't have them
// Don't use .size(), clang's ranges doesn't have it

template<typename C>
bool contains(const C &container, const typename C::value_type &value)
{
   return std::find(container.begin(), container.end(), value) != container.end();
}

inline bool contains(const std::string &haystack, const std::string &needle)
{
   return haystack.find(needle) != std::string::npos;
}

template<typename C, typename Pred>
bool contains_if(const C &container, Pred pred)
{
   return std::find_if(container.begin(), container.end(), pred) != container.end();
}

template<typename C>
typename C::iterator find(C &container, const typename C::value_type &value)
{
    return std::find(container.begin(), container.end(), value);
}

template<typename C>
typename C::const_iterator find(const C &container, const typename C::value_type &value)
{
    return std::find(container.begin(), container.end(), value);
}

template<typename C, typename Pred>
typename C::iterator find_if(C &container, Pred pred)
{
    return std::find_if(container.begin(), container.end(), pred);
}

template<typename C, typename Pred>
typename C::const_iterator find_if(const C &container, Pred pred)
{
    return std::find_if(container.begin(), container.end(), pred);
}

template<typename Range, typename Pred>
bool any_of(const Range &r, Pred pred)
{
    return std::any_of(r.begin(), r.end(), pred);
}

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 7

    template<typename Pred>
    bool any_of(const clang::StmtRange &r, Pred pred)
    {
        return std::any_of(r.first, r.second, pred);
    }

#endif


template<typename Range, typename Pred>
bool all_of(const Range &r, Pred pred)
{
    return std::all_of(r.begin(), r.end(), pred);
}

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 7

    template<typename Pred>
    bool all_of(const clang::StmtRange &r, Pred pred)
    {
        return std::all_of(r.first, r.second, pred);
    }

#endif

template <typename Range>
size_t count(const Range &r)
{
    return std::distance(r.begin(), r.end());
}

template<typename SrcContainer, typename DstContainer>
void copy(const SrcContainer &src, DstContainer &dst)
{
    dst.reserve(clazy_std::count(dst) + clazy_std::count(src));
    std::copy(src.begin(), src.end(), std::back_inserter(dst));
}

template<typename SrcContainer, typename DstContainer, typename Pred>
void copy_if(const SrcContainer &src, DstContainer &dst, Pred pred)
{
    dst.reserve(clazy_std::count(dst) + clazy_std::count(src));
    std::copy_if(src.begin(), src.end(), std::back_inserter(dst), pred);
}

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 7
    template <typename Range>
    bool isEmpty(const Range &r)
    {
        return r.begin() == r.end();
    }
#else
    inline bool isEmpty(const clang::StmtRange &r)
    {
        return r.empty();
    }
#endif

inline bool hasChildren(clang::Stmt *s)
{
    return s && !clazy_std::isEmpty(s->children());
}

/**
 * Returns true if the string target starts with maybeBeginning
 */
inline bool startsWith(const std::string &target, const std::string &maybeBeginning)
{
    return target.compare(0, maybeBeginning.length(), maybeBeginning) == 0;
}

/**
 * Returns true if the string target starts with any of the strings in beginningCandidates
 */
inline bool startsWithAny(const std::string &target, const std::vector<std::string> &beginningCandidates)
{
    return clazy_std::any_of(beginningCandidates, [target](const std::string &maybeBeginning) {
        return clazy_std::startsWith(target, maybeBeginning);
    });
}

/**
 * Returns true if the target equals any of the candidate strings
 */
inline bool equalsAny(const std::string &target, const std::vector<std::string> &candidates)
{
    return clazy_std::any_of(candidates, [target](const std::string &candidate) {
        return candidate == target;
    });
}

/**
 * Returns true if the string target ends with maybeEnding
 */
inline bool endsWith(const std::string &target, const std::string &maybeEnding)
{
    return target.size() >= maybeEnding.size() &&
            target.compare(target.size() - maybeEnding.size(), maybeEnding.size(), maybeEnding) == 0;
}

/**
 * Returns true if the string target ends with any of the strings in endingCandidates
 */
inline bool endsWithAny(const std::string &target, const std::vector<std::string> &endingCandidates)
{
    return clazy_std::any_of(endingCandidates, [target](const std::string &maybeEnding) {
        return clazy_std::endsWith(target, maybeEnding);
    });
}


inline std::string toLower(const std::string &s)
{
    std::string result(s.size(), 0);
    std::transform(s.begin(), s.end(), result.begin(), ::tolower);
    return result;
}


inline std::vector<std::string> splitString(const std::string &str, char separator)
{
    std::string token;
    std::vector<std::string> result;
    std::istringstream istream(str);
    while (std::getline(istream, token, separator)) {
        result.push_back(token);
    }

    return result;
}

inline std::vector<std::string> splitString(const char *str, char separator)
{
    if (!str)
        return {};

    return clazy_std::splitString(std::string(str), separator);
}

}

#endif
