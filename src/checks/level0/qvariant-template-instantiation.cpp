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

#include "qvariant-template-instantiation.h"
#include "TemplateUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <ctype.h>
#include <memory>
#include <vector>

class ClazyContext;

using namespace std;
using namespace clang;

QVariantTemplateInstantiation::QVariantTemplateInstantiation(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isMatchingClass(StringRef name)
{
    static const vector<StringRef> classes = {"QBitArray", "QByteArray", "QChar", "QDate", "QDateTime",
                                              "QEasingCurve", "QJsonArray", "QJsonDocument", "QJsonObject",
                                              "QJsonValue", "QLocale", "QModelIndex", "QPoint", "QPointF",
                                              "QRect", "QRectF", "QRegExp", "QString", "QRegularExpression",
                                              "QSize", "QSizeF", "QStringList", "QTime", "QUrl", "QUuid" };

    return clazy::contains(classes, name);
}

void QVariantTemplateInstantiation::VisitStmt(clang::Stmt *stm)
{
    auto callExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (!callExpr)
        return;

    CXXMethodDecl *methodDecl = callExpr->getMethodDecl();
    if (!methodDecl || clazy::name(methodDecl) != "value")
        return;

    CXXRecordDecl *decl = methodDecl->getParent();
    if (!decl || clazy::name(decl) != "QVariant")
        return;

    vector<QualType> typeList = clazy::getTemplateArgumentsTypes(methodDecl);
    const Type *t = typeList.empty() ? nullptr : typeList[0].getTypePtrOrNull();
    if (!t)
        return;

    bool matches = false;
    if (t->isBooleanType() || t->isFloatingType() || (t->isIntegerType() && !t->isEnumeralType())) {
        matches = true;
    } else {
        CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
        matches = recordDecl && t->isClassType() && isMatchingClass(clazy::name(recordDecl));
    }

    if (matches) {
        string typeName = clazy::simpleTypeName(typeList[0], lo());

        string typeName2 = typeName;
        typeName2[0] = toupper(typeName2[0]);


        if (typeName[0] == 'Q')
            typeName2.erase(0, 1); // Remove first letter
        std::string error = std::string("Use QVariant::to" + typeName2 + "() instead of QVariant::value<" + typeName + ">()");
        emitWarning(clazy::getLocStart(stm), error.c_str());
    }
}
