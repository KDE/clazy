/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

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

#ifndef OLD_STYLE_CONNECT_H
#define OLD_STYLE_CONNECT_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>
#include <vector>

class ClazyContext;

namespace clang {
class CallExpr;
class CXXMemberCallExpr;
class Expr;
class FixItHint;
class FunctionDecl;
class MacroInfo;
class Stmt;
class Token;
}

struct PrivateSlot
{
    typedef std::vector<PrivateSlot> List;
    std::string objName;
    std::string name;
};

/**
 * Finds usages of old-style Qt connect statements.
 *
 * See README-old-style-connect for more information.
 */
class OldStyleConnect
    : public CheckBase
{
public:
    OldStyleConnect(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;
    void addPrivateSlot(const PrivateSlot &);
protected:
    void VisitMacroExpands(const clang::Token &macroNameTok, const clang::SourceRange &, const clang::MacroInfo *minfo = nullptr) override;
private:
    std::string signalOrSlotNameFromMacro(clang::SourceLocation macroLoc);

    template <typename T>
    std::vector<clang::FixItHint> fixits(int classification, T *callOrCtor);

    bool isSignalOrSlot(clang::SourceLocation loc, std::string &macroName) const;

    template <typename T>
    int classifyConnect(clang::FunctionDecl *connectFunc, T *connectCall) const;

    bool isQPointer(clang::Expr *expr) const;
    bool isPrivateSlot(const std::string &name) const;
    PrivateSlot::List m_privateSlots;
};

#endif
