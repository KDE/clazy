/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QSTRING_ARG_H
#define QSTRING_ARG_H

#include "checkbase.h"

#include <string>
#include <vector>

namespace clang
{
class CXXMemberCallExpr;
class CallExpr;
}

/**
 * Finds misuse of QString::arg()
 */
class QStringArg : public CheckBase
{
public:
    explicit QStringArg(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
    void checkForMultiArgOpportunities(clang::CXXMemberCallExpr *memberCall);

private:
    bool checkQLatin1StringCase(clang::CXXMemberCallExpr *);
    bool checkMultiArgWarningCase(const std::vector<clang::CallExpr *> &calls);
    std::vector<clang::CallExpr *> m_alreadyProcessedChainedCalls;
};

#endif
