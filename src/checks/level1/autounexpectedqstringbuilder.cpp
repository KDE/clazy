/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>
  Copyright (C) 2015 Mathias Hasselmann <mathias.hasselmann@kdab.com>

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

#include "autounexpectedqstringbuilder.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "FixItUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


enum Fixit {
    FixitNone = 0,
    FixitUseQString = 0x1,
};

AutoUnexpectedQStringBuilder::AutoUnexpectedQStringBuilder(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}


void AutoUnexpectedQStringBuilder::VisitDecl(Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl)
        return;

    const Type *type = varDecl->getType().getTypePtrOrNull();
    if (!type || !type->isRecordType() || !dyn_cast<AutoType>(type))
        return;

    CXXRecordDecl *record = type->getAsCXXRecordDecl();
    if (record && record->getName() == "QStringBuilder") {
        std::vector<FixItHint> fixits;

        if (isFixitEnabled(FixitUseQString)) {
            std::string replacement = "QString " + varDecl->getName().str();

            if (varDecl->getType().isConstQualified())
                replacement = "const " + replacement;

            SourceLocation start = varDecl->getLocStart();
            SourceLocation end = varDecl->getLocation();
            fixits.push_back(FixItUtils::createReplacement({ start, end }, replacement));
        }

        emitWarning(decl->getLocStart(), "auto deduced to be QStringBuilder instead of QString. Possible crash.", fixits);
    }
}

const char *const s_checkName = "auto-unexpected-qstringbuilder";
REGISTER_CHECK(s_checkName, AutoUnexpectedQStringBuilder, CheckLevel1)
REGISTER_FIXIT(FixitUseQString, "fix-auto-unexpected-qstringbuilder", s_checkName)
