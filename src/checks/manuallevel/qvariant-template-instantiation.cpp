/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qvariant-template-instantiation.h"
#include "StringUtils.h"
#include "TemplateUtils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <ctype.h>
#include <vector>

class ClazyContext;

using namespace clang;

QVariantTemplateInstantiation::QVariantTemplateInstantiation(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isMatchingClass(StringRef name)
{
    static const std::vector<StringRef> classes = {"QBitArray",     "QByteArray",  "QChar",      "QDate",   "QDateTime",          "QEasingCurve", "QJsonArray",
                                                   "QJsonDocument", "QJsonObject", "QJsonValue", "QLocale", "QModelIndex",        "QPoint",       "QPointF",
                                                   "QRect",         "QRectF",      "QRegExp",    "QString", "QRegularExpression", "QSize",        "QSizeF",
                                                   "QStringList",   "QTime",       "QUrl",       "QUuid"};

    return clazy::contains(classes, name);
}

void QVariantTemplateInstantiation::VisitStmt(clang::Stmt *stm)
{
    auto *callExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (!callExpr) {
        return;
    }

    CXXMethodDecl *methodDecl = callExpr->getMethodDecl();
    if (!methodDecl || clazy::name(methodDecl) != "value") {
        return;
    }

    const auto *memberExpr = dyn_cast<MemberExpr>(callExpr->getCallee());
    if (!memberExpr) {
        return;
    }

    const auto *decl = dyn_cast<CXXRecordDecl>(memberExpr->getBase()->getType()->getAsCXXRecordDecl());
    if (!decl || !decl->getDefinition()
        || !(decl->getNameAsString() == "QVariant" || decl->getNameAsString() == "QHash" || decl->getNameAsString() == "QMap")) {
        return;
    }

    if (const auto *specDecl = dyn_cast<ClassTemplateSpecializationDecl>(decl)) {
        if (const auto *templateDecl = specDecl->getSpecializedTemplate()) {
            if (templateDecl->getNameAsString() == "QHash" || templateDecl->getNameAsString() == "QMap") {
                const TemplateArgumentList &templateArgs = specDecl->getTemplateArgs();
                if (!(templateArgs.size() == 2) || !(templateArgs.get(1).getAsType().getAsString() == "QVariant")) {
                    return;
                }
            } else if (templateDecl->getNameAsString() == "QList") {
                const TemplateArgumentList &templateArgs = specDecl->getTemplateArgs();
                if (!(templateArgs.size() == 1) || !(templateArgs.get(0).getAsType().getAsString() == "QVariant")) {
                    return;
                }
            }
        }
    }

    std::vector<QualType> typeList = clazy::getTemplateArgumentsTypes(methodDecl);
    const Type *t = typeList.empty() ? nullptr : typeList[0].getTypePtrOrNull();
    if (!t) {
        return;
    }

    bool matches = false;
    if (t->isBooleanType() || t->isFloatingType() || (t->isIntegerType() && !t->isEnumeralType())) {
        matches = true;
    } else {
        const CXXRecordDecl *recordDecl = t->getAsCXXRecordDecl();
        matches = recordDecl && t->isClassType() && isMatchingClass(clazy::name(recordDecl));
    }

    if (matches) {
        std::string typeName = clazy::simpleTypeName(typeList[0], lo());

        std::string typeName2 = typeName;
        typeName2[0] = toupper(typeName2[0]);

        if (typeName[0] == 'Q') {
            typeName2.erase(0, 1); // Remove first letter
        }
        std::string error = std::string("Use QVariant::to" + typeName2 + "() instead of QVariant::value<" + typeName + ">()");
        emitWarning(stm->getBeginLoc(), error);
    }
}
