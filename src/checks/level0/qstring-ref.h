/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#ifndef QSTRING_REF_H
#define QSTRING_REF_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang {
class Stmt;
class CallExpr;
class CXXMemberCallExpr;
class FixItHint;
}

/**
 * Finds places where the QString::fooRef() should be used instead QString::foo(), to save allocations
 *
 * See README-qstringref for more info.
 */
class StringRefCandidates
    : public CheckBase
{
public:
    StringRefCandidates(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
private:
    bool processCase1(clang::CXXMemberCallExpr*);
    bool processCase2(clang::CallExpr *call);
    bool isConvertedToSomethingElse(clang::Stmt* s) const;

    std::vector<clang::CallExpr*> m_alreadyProcessedChainedCalls;
    std::vector<clang::FixItHint> fixit(clang::CXXMemberCallExpr*);
};

#endif
