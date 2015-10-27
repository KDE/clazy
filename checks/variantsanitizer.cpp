/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "variantsanitizer.h"
#include "Utils.h"
#include "checkmanager.h"

using namespace std;
using namespace clang;

VariantSanitizer::VariantSanitizer(const std::string &name)
    : CheckBase(name)
{

}

static bool isMatchingClass(const std::string &name)
{
    static const vector<string> classes = {"QBitArray", "QByteArray", "QChar", "QDate", "QDateTime",
                                           "QEasingCurve", "QJsonArray", "QJsonDocument", "QJsonObject",
                                           "QJsonValue", "QLocale", "QModelIndex", "QPoint", "QPointF",
                                           "QRect", "QRectF", "QRegExp", "QString", "QRegularExpression",
                                           "QSize", "QSizeF", "QStringList", "QTime", "QUrl", "QUuid" };

    return find(classes.cbegin(), classes.cend(), name) != classes.cend();
}

void VariantSanitizer::VisitStmt(clang::Stmt *stm)
{
    auto callExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (callExpr == nullptr)
        return;

    CXXMethodDecl *methodDecl = callExpr->getMethodDecl();
    if (methodDecl == nullptr || methodDecl->getNameAsString() != "value")
        return;

    CXXRecordDecl *decl = methodDecl->getParent();
    if (decl == nullptr || decl->getNameAsString() != "QVariant")
        return;

    FunctionTemplateSpecializationInfo *specializationInfo = methodDecl->getTemplateSpecializationInfo();
    if (specializationInfo == nullptr || specializationInfo->TemplateArguments == nullptr || specializationInfo->TemplateArguments->size() != 1)
        return;

    const TemplateArgument &argument = specializationInfo->TemplateArguments->get(0);
    QualType qt = argument.getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (t == nullptr)
        return;

    bool matches = false;
    if (t->isBooleanType() /*|| t->isIntegerType() || t->isFloatingType()*/) {
        matches = true;
    } else {
        CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
        matches = t->isClassType() && recordDecl && isMatchingClass(recordDecl->getNameAsString());
    }

    if (matches) {
        std::string error = std::string("Use QVariant::toFoo() instead of QVariant::value<Foo>()");
        emitWarning(stm->getLocStart(), error.c_str());
    }
}

REGISTER_CHECK_WITH_FLAGS("variant-sanitizer", VariantSanitizer, CheckLevel0)
