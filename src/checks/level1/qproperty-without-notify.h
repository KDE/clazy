/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QPROPERTY_WITHOUT_NOTIFY_H
#define CLAZY_QPROPERTY_WITHOUT_NOTIFY_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>

class ClazyContext;
namespace clang
{
class MacroInfo;
class Token;
} // namespace clang

/**
 * See README-qproperty-without-notify.md for more info.
 */
class QPropertyWithoutNotify : public CheckBase
{
public:
    explicit QPropertyWithoutNotify(const std::string &name, ClazyContext *context);

private:
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;

    bool m_lastIsGadget = false;
};

#endif
