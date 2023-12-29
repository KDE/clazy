/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OLD_STYLE_CONNECT_H
#define OLD_STYLE_CONNECT_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
class CallExpr;
class CXXMemberCallExpr;
class Expr;
class FixItHint;
class FunctionDecl;
class MacroInfo;
class Stmt;
class Token;
}

struct PrivateSlot {
    using List = std::vector<PrivateSlot>;
    std::string objName;
    std::string name;
};

/**
 * Finds usages of old-style Qt connect statements.
 *
 * See README-old-style-connect for more information.
 */
class OldStyleConnect : public CheckBase
{
public:
    OldStyleConnect(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;
    void addPrivateSlot(const PrivateSlot &);

protected:
    void VisitMacroExpands(const clang::Token &macroNameTok, const clang::SourceRange &, const clang::MacroInfo *minfo = nullptr) override;

private:
    std::string signalOrSlotNameFromMacro(clang::SourceLocation macroLoc);

    template<typename T>
    std::vector<clang::FixItHint> fixits(int classification, T *callOrCtor);

    bool isSignalOrSlot(clang::SourceLocation loc, std::string &macroName) const;

    template<typename T>
    int classifyConnect(clang::FunctionDecl *connectFunc, T *connectCall) const;

    bool isQPointer(clang::Expr *expr) const;
    bool isPrivateSlot(const std::string &name) const;
    PrivateSlot::List m_privateSlots;
};

#endif
