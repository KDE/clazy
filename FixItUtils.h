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

#ifndef CLAZY_FIXIT_UTILS_H
#define CLAZY_FIXIT_UTILS_H

#include <string>

namespace clang {
class FixItHint;
class SourceRange;
class SourceLocation;
class StringLiteral;
}

namespace FixItUtils {

clang::FixItHint createReplacement(const clang::SourceRange &range, const std::string &replacement);
clang::FixItHint createInsertion(const clang::SourceLocation &start, const std::string &insertion);

clang::SourceRange rangeForLiteral(clang::StringLiteral *);

}

#endif

