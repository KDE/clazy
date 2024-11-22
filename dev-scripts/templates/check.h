/*
    %4

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_%1_H
#define CLAZY_%1_H

#include "checkbase.h"

/**
 * See README-%3.md for more info.
 */
class %2 : public CheckBase
{
public:
    explicit %2(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;
    void VisitStmt(clang::Stmt *) override;
private:
};

#endif
