/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "variantsanitizer.h"
#include "Utils.h"

using namespace std;
using namespace clang;

VariantSanitizer::VariantSanitizer(clang::CompilerInstance &ci)
    : CheckBase(ci)
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

std::string VariantSanitizer::name() const
{
    return "variant-sanitizer";
}
