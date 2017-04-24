/*
   This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "unused-non-trivial-variable.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"
#include "ContextUtils.h"
#include "QtUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

#include <vector>
#include <string>

using namespace clang;
using namespace std;


UnusedNonTrivialVariable::UnusedNonTrivialVariable(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void UnusedNonTrivialVariable::VisitStmt(clang::Stmt *stmt)
{
    auto declStmt = dyn_cast<DeclStmt>(stmt);
    if (!declStmt)
        return;

    for (auto decl : declStmt->decls())
        handleVarDecl(dyn_cast<VarDecl>(decl));
}

bool UnusedNonTrivialVariable::isInterestingType(QualType t) const
{
    // TODO Remove QColor in Qt6
    static const vector<string> nonTrivialTypes = { "QColor", "QVariant", "QFont", "QUrl", "QIcon",
                                                    "QImage", "QPixmap", "QPicture", "QBitmap", "QBrush",
                                                    "QPen", "QBuffer", "QCache", "QDateTime", "QDir", "QEvent",
                                                    "QFileInfo", "QFontInfo", "QFontMetrics", "QJSValue", "QLocale",
                                                    "QRegularExpression", "QRegExp"};

    if (QtUtils::isQtContainer(t, lo()))
        return true;

    const string typeName = StringUtils::simpleTypeName(t, lo());
    return clazy_std::any_of(nonTrivialTypes, [typeName] (const string &container) {
        return container == typeName;
    });
}

void UnusedNonTrivialVariable::handleVarDecl(VarDecl *varDecl)
{
    if (!varDecl || !isInterestingType(varDecl->getType()))
        return;

    auto currentFunc = ContextUtils::firstContextOfType<FunctionDecl>(varDecl->getDeclContext());
    Stmt *body = currentFunc ? currentFunc->getBody() : nullptr;
    if (!body)
        return;

    SourceLocation locStart = varDecl->getLocStart();
    auto declRefs = HierarchyUtils::getStatements<DeclRefExpr>(body, &sm(), locStart);

    auto pred = [varDecl] (DeclRefExpr *declRef) {
        return declRef->getDecl() == varDecl;
    };

    if (!clazy_std::any_of(declRefs, pred))
        emitWarning(locStart, "unused " + StringUtils::simpleTypeName(varDecl->getType(), lo()));
}


REGISTER_CHECK_WITH_FLAGS("unused-non-trivial-variable", UnusedNonTrivialVariable, CheckLevel0)
