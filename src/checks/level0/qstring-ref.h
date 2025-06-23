/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QSTRING_REF_H
#define QSTRING_REF_H

#include "checkbase.h"

#include <clang/Basic/Diagnostic.h>
#include <string>
#include <vector>

namespace clang
{
class CallExpr;
class CXXMemberCallExpr;
}

/**
 * Finds places where the QString::fooRef() should be used instead QString::foo(), to save allocations
 *
 * See README-qstringref for more info.
 */
class StringRefCandidates : public CheckBase
{
public:
    StringRefCandidates(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool processCase1(clang::CXXMemberCallExpr *);
    bool processCase2(clang::CallExpr *call);
    bool isConvertedToSomethingElse(clang::Stmt *s) const;

    std::vector<clang::CallExpr *> m_alreadyProcessedChainedCalls;
    std::vector<clang::FixItHint> fixit(clang::CXXMemberCallExpr *);
};

#endif
