/*
    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "unused-non-trivial-variable.h"
#include "ContextUtils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <stdlib.h>
#include <string>
#include <vector>

using namespace clang;

UnusedNonTrivialVariable::UnusedNonTrivialVariable(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    const char *user_blacklist = getenv("CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_BLACKLIST");
    const char *user_whitelist = getenv("CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_WHITELIST");

    if (user_blacklist) {
        m_userBlacklist = clazy::splitString(user_blacklist, ',');
    }

    if (user_whitelist) {
        m_userWhitelist = clazy::splitString(user_whitelist, ',');
    }
}

void UnusedNonTrivialVariable::VisitStmt(clang::Stmt *stmt)
{
    auto *declStmt = dyn_cast<DeclStmt>(stmt);
    if (!declStmt) {
        return;
    }

    for (auto *decl : declStmt->decls()) {
        handleVarDecl(dyn_cast<VarDecl>(decl));
    }
}

bool UnusedNonTrivialVariable::isUninterestingType(const CXXRecordDecl *record) const
{
    static const std::vector<StringRef> blacklist = {
        "QMutexLocker",
        "QDebugStateSaver",
        "QTextBlockFormat",
        "QWriteLocker",
        "QSignalBlocker",
        "QReadLocker",
        "PRNGLocker",
        "QDBusWriteLocker",
        "QDBusBlockingCallWatcher",
        "QBoolBlocker",
        "QOrderedMutexLocker",
        "QTextLine",
        "QScopedScopeLevelCounter",
    };

    // Check some obvious candidates first
    StringRef typeName = clazy::name(record);
    bool any = clazy::any_of(blacklist, [typeName](StringRef container) {
        return container == typeName;
    });

    if (any) {
        return true;
    }

    static const std::vector<StringRef> blacklistedTemplates = {
        "QScopedPointer",
        "QSetValueOnDestroy",
        "QScopedValueRollback",
        "QScopeGuard",
    };
    StringRef className = clazy::name(record);
    for (StringRef templateName : blacklistedTemplates) {
        if (clazy::startsWith(static_cast<std::string>(className), static_cast<std::string>(templateName))) {
            return true;
        }
    }

    // Now check the user's blacklist, set by env-variable
    any = clazy::any_of(m_userBlacklist, [typeName](const std::string &container) {
        return container == typeName;
    });

    return any;
}

bool UnusedNonTrivialVariable::isInterestingType(QualType t) const
{
    // TODO Remove QColor in Qt6
    static const std::vector<StringRef> nonTrivialTypes = {
        "QColor",
        "QVariant",
        "QFont",
        "QUrl",
        "QIcon",
        "QImage",
        "QPixmap",
        "QPicture",
        "QBitmap",
        "QBrush",
        "QPen",
        "QBuffer",
        "QCache",
        "QDateTime",
        "QDir",
        "QEvent",
        "QFileInfo",
        "QFontInfo",
        "QFontMetrics",
        "QJSValue",
        "QLocale",
        "QRegularExpression",
        "QRegExp",
        "QUrlQuery",
        "QStorageInfo",
        "QPersistentModelIndex",
        "QJsonArray",
        "QJsonValue",
        "QJsonDocument",
        "QMimeType",
        "QBitArray",
        "QCollator",
        "QByteArrayList",
        "QCollatorSortKey",
        "QCursor",
        "QPalette",
        "QPainterPath",
        "QRegion",
        "QFontInfo",
        "QTextCursor",
        "QStaticText",
        "QFontMetricsF",
        "QTextFrameFormat",
        "QTextImageFormat",
        "QNetworkCookie",
        "QNetworkRequest",
        "QNetworkConfiguration",
        "QHostAddress",
        "QSqlQuery",
        "QSqlRecord",
        "QSqlField",
        "QLine",
        "QLineF",
        "QRect",
        "QRectF",
        "QDomNode",
    };

    const CXXRecordDecl *record = clazy::typeAsRecord(t);
    if (!record) {
        return false;
    }

    if (isOptionSet("no-whitelist")) {
        // Will cause too many false-positives, like RAII classes. Use suppressing comments to silence them.
        return !isUninterestingType(record);
    }

    if (clazy::isQtContainer(record)) {
        return true;
    }

    StringRef typeName = clazy::name(record);
    bool any = clazy::any_of(nonTrivialTypes, [typeName](StringRef container) {
        return container == typeName;
    });

    if (any) {
        return true;
    }

    return clazy::any_of(m_userWhitelist, [typeName](const std::string &container) {
        return container == typeName;
    });
}

void UnusedNonTrivialVariable::handleVarDecl(VarDecl *varDecl)
{
    if (!varDecl || !isInterestingType(varDecl->getType())) {
        return;
    }

    auto *currentFunc = clazy::firstContextOfType<FunctionDecl>(varDecl->getDeclContext());
    Stmt *body = currentFunc ? currentFunc->getBody() : nullptr;
    if (!body) {
        return;
    }

    SourceLocation locStart = varDecl->getBeginLoc();
    locStart = sm().getExpansionLoc(locStart);
    auto declRefs = clazy::getStatements<DeclRefExpr>(body, &sm(), locStart);

    auto pred = [varDecl](DeclRefExpr *declRef) {
        return declRef->getDecl() == varDecl;
    };

    if (!clazy::any_of(declRefs, pred)) {
        // Check for [[maybe_unused]] attribute
        if (!varDecl->hasAttr<clang::UnusedAttr>()) {
            const std::string varName = varDecl->getDeclName().getAsString();
            emitWarning(locStart, "unused " + clazy::simpleTypeName(varDecl->getType(), lo()) + " " + varName);
        }
    }
}
