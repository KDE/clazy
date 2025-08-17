/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT6_QHASH_SIGNATURE
#define CLAZY_QT6_QHASH_SIGNATURE

#include "checkbase.h"

#include <vector>

namespace clang
{
class FixItHint;
}

/**
 * Replaces qhash signature uint with size_t.
 *
 * Run only in Qt 6 code.
 */
class Qt6QHashSignature : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    std::vector<clang::FixItHint> fixitReplace(clang::FunctionDecl *funcDecl, bool changeReturnType, bool changeParamType);
};

#endif
