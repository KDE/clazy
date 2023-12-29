/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_MISSING_Q_OBJECT_H
#define CLANG_MISSING_Q_OBJECT_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
class Decl;
class SourceLocation;
class MacroInfo;
class Token;
}

/**
 * Finds QObject derived classes that don't have a Q_OBJECT macro
 *
 * See README-missing-qobject for more information
 */
class MissingQObjectMacro : public CheckBase
{
public:
    explicit MissingQObjectMacro(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;
    void registerQ_OBJECT(clang::SourceLocation);
    std::vector<clang::SourceLocation> m_qobjectMacroLocations;

    bool m_hasAddedMocFile = false;
};

#endif
