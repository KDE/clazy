/*
   This file is part of the clazy static checker.

  Copyright (C) 2016-2017 Sergio Martins <smartins@kde.org>

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

#include "StringUtils.h"
#include "HierarchyUtils.h"
#include "ContextUtils.h"
#include "QtUtils.h"
#include "clazy_stl.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

#include <vector>
#include <string>

using namespace clang;
using namespace std;


UnusedNonTrivialVariable::UnusedNonTrivialVariable(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
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

bool UnusedNonTrivialVariable::isUninterestingType(const CXXRecordDecl *record) const
{
    static const vector<StringRef> blacklist = { "QMutexLocker", "QDebugStateSaver",
                                                 "QTextBlockFormat", "QWriteLocker",
                                                 "QSignalBlocker", "QReadLocker", "PRNGLocker", "QDBusWriteLocker", "QDBusBlockingCallWatcher",
                                                 "QBoolBlocker", "QOrderedMutexLocker", "QTextLine", "QScopedScopeLevelCounter" };

    StringRef typeName = clazy::name(record);
    bool any = clazy::any_of(blacklist, [typeName] (StringRef container) {
        return container == typeName;
    });

    if (any)
        return true;

    static const vector<StringRef> blacklistedTemplates = { "QScopedPointer", "QSetValueOnDestroy", "QScopedValueRollback" };
    StringRef className = clazy::name(record);
    for (StringRef templateName : blacklistedTemplates) {
        if (clazy::startsWith(className, templateName))
            return true;
    }

    return false;
}

bool UnusedNonTrivialVariable::isInterestingType(QualType t) const
{
    // TODO Remove QColor in Qt6
    static const vector<StringRef> nonTrivialTypes = { "QColor", "QVariant", "QFont", "QUrl", "QIcon",
                                                       "QImage", "QPixmap", "QPicture", "QBitmap", "QBrush",
                                                       "QPen", "QBuffer", "QCache", "QDateTime", "QDir", "QEvent",
                                                       "QFileInfo", "QFontInfo", "QFontMetrics", "QJSValue", "QLocale",
                                                       "QRegularExpression", "QRegExp", "QUrlQuery", "QStorageInfo",
                                                       "QPersistentModelIndex", "QJsonArray", "QJsonDocument",
                                                       "QMimeType", "QBitArray", "QCollator",
                                                       "QByteArrayList", "QCollatorSortKey",
                                                       "QCursor", "QPalette", "QPainterPath", "QRegion", "QFontInfo", "QTextCursor",
                                                       "QStaticText", "QFontMetricsF", "QTextFrameFormat", "QTextImageFormat",
                                                       "QNetworkCookie", "QNetworkRequest", "QNetworkConfiguration",
                                                       "QHostAddress", "QSqlQuery", "QSqlRecord", "QSqlField",
                                                       "QLine", "QLineF", "QRect", "QRectF"
                                                     };

    CXXRecordDecl *record = TypeUtils::typeAsRecord(t);
    if (!record)
        return false;

    if (isOptionSet("no-whitelist")) {
        // Will cause too many false-positives, like RAII classes. Use suppressing comments to silence them.
        return !isUninterestingType(record);
    }

    if (clazy::isQtContainer(record))
        return true;

    StringRef typeName = clazy::name(record);
    return clazy::any_of(nonTrivialTypes, [typeName] (StringRef container) {
        return container == typeName;
    });
}

void UnusedNonTrivialVariable::handleVarDecl(VarDecl *varDecl)
{
    if (!varDecl || !isInterestingType(varDecl->getType()))
        return;

    auto currentFunc = clazy::firstContextOfType<FunctionDecl>(varDecl->getDeclContext());
    Stmt *body = currentFunc ? currentFunc->getBody() : nullptr;
    if (!body)
        return;

    SourceLocation locStart = varDecl->getLocStart();
    locStart = sm().getExpansionLoc(locStart);
    auto declRefs = clazy::getStatements<DeclRefExpr>(body, &sm(), locStart);

    auto pred = [varDecl] (DeclRefExpr *declRef) {
        return declRef->getDecl() == varDecl;
    };

    if (!clazy::any_of(declRefs, pred))
        emitWarning(locStart, "unused " + clazy::simpleTypeName(varDecl->getType(), lo()));
}
