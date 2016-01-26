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

#include "variantsanitizer.h"
#include "Utils.h"
#include "TemplateUtils.h"
#include "checkmanager.h"

using namespace std;
using namespace clang;

VariantSanitizer::VariantSanitizer(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{

}

static bool isMatchingClass(const std::string &name, const clang::CompilerInstance &ci)
{
    static const vector<string> classes = {"QBitArray", "QByteArray", "QChar", "QDate", "QDateTime",
                                           "QEasingCurve", "QJsonArray", "QJsonDocument", "QJsonObject",
                                           "QJsonValue", "QLocale", "QModelIndex", "QPoint", "QPointF",
                                           "QRect", "QRectF", "QRegExp", "QString", "QRegularExpression",
                                           "QSize", "QSizeF", "QStringList", "QTime", "QUrl", "QUuid" };

    return clazy_std::contains(classes, name);
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

    vector<QualType> typeList = TemplateUtils::getTemplateArgumentsTypes(methodDecl);
    const Type *t = typeList.empty() ? nullptr : typeList[0].getTypePtrOrNull();
    if (!t)
        return;

    bool matches = false;
    if (t->isBooleanType() /*|| t->isIntegerType() || t->isFloatingType()*/) {
        matches = true;
    } else {
        CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
        matches = t->isClassType() && recordDecl && isMatchingClass(recordDecl->getNameAsString(), m_ci);
    }

    if (matches) {
        std::string error = std::string("Use QVariant::toFoo() instead of QVariant::value<Foo>()");
        emitWarning(stm->getLocStart(), error.c_str());
    }
}

REGISTER_CHECK_WITH_FLAGS("variant-sanitizer", VariantSanitizer, CheckLevel0)
