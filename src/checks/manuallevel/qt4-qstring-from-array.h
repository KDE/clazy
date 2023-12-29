/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT4_QSTRING_FROM_ARRAY_H
#define CLAZY_QT4_QSTRING_FROM_ARRAY_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
class Stmt;
class FixItHint;
class CXXConstructExpr;
class CXXOperatorCallExpr;
class Expr;
class CXXMemberCallExpr;
}

/**
 * Replaces QString(char*) or QString(QByteArray) calls with QString::fromLatin1().
 *
 * Run only in Qt 4 code.
 */
class Qt4QStringFromArray : public CheckBase
{
public:
    explicit Qt4QStringFromArray(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    std::vector<clang::FixItHint> fixCtorCall(clang::CXXConstructExpr *ctorExpr);
    std::vector<clang::FixItHint> fixOperatorCall(clang::CXXOperatorCallExpr *ctorExpr);
    std::vector<clang::FixItHint> fixMethodCallCall(clang::CXXMemberCallExpr *memberExpr);
    std::vector<clang::FixItHint> fixitReplaceWithFromLatin1(clang::CXXConstructExpr *ctorExpr);
    std::vector<clang::FixItHint> fixitInsertFromLatin1(clang::CXXConstructExpr *ctorExpr);
};

#endif
