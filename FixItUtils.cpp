/*
  This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "FixItUtils.h"
#include "checkmanager.h"

#include <clang/AST/Expr.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Lexer.h>

using namespace FixItUtils;
using namespace clang;

clang::FixItHint FixItUtils::createReplacement(const clang::SourceRange &range, const std::string &replacement)
{
    if (range.getBegin().isInvalid()) {
        return {};
    } else {
        return FixItHint::CreateReplacement(range, replacement);
    }
}

clang::FixItHint FixItUtils::createInsertion(const clang::SourceLocation &start, const std::string &insertion)
{
    if (start.isInvalid()) {
        return {};
    } else {
        return FixItHint::CreateInsertion(start, insertion);
    }
}

SourceRange FixItUtils::rangeForLiteral(StringLiteral *lt)
{
    if (!lt)
        return {};

    const int numTokens = lt->getNumConcatenated();
    const SourceLocation lastTokenLoc = lt->getStrTokenLoc(numTokens - 1);
    if (lastTokenLoc.isInvalid()) {
        return {};
    }

    SourceRange range;
    range.setBegin(lt->getLocStart());

    SourceLocation end = Lexer::getLocForEndOfToken(lastTokenLoc, 0,
                                                    CheckManager::instance()->m_ci->getSourceManager(),
                                                    CheckManager::instance()->m_ci->getLangOpts()); // For some reason lt->getLocStart() is == to lt->getLocEnd()

    if (!end.isValid()) {
        return {};
    }

    range.setEnd(end);
    return range;
}
