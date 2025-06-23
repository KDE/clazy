/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "requiredresults.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;

RequiredResults::RequiredResults(const std::string &name)
    : CheckBase(name)
{
}

bool RequiredResults::shouldIgnoreMethod(const StringRef &qualifiedName)
{
    static const std::vector<StringRef> files = {
        "QDir::mkdir",
        "QDir::rmdir",
        "QDir::mkpath",
        "QDBusConnection::send",

        "QRegExp::indexIn",
        "QRegExp::exactMatch",
        "QQmlProperty::write",
        "QQmlProperty::reset",
        "QWidget::winId",
        "QtWaylandClient::QWaylandEglWindow::contentFBO",
        "ProString::updatedHash",

        // kdepim
        "KCalCore::Incidence::recurrence",
        "KCalCore::RecurrenceRule::Private::buildCache",
        "KAlarmCal::KAEvent::updateKCalEvent",
        "Akonadi::Server::Collection::clearMimeTypes",
        "Akonadi::Server::Collection::addMimeType",
        "Akonadi::Server::PimItem::addFlag",
        "Akonadi::Server::PimItem::addTag",

        // kf5 libs
        "KateVi::Command::execute",
        "KArchiveDirectory::copyTo",
        "KBookmarkManager::saveAs",
        "KBookmarkManager::save",
        "KLineEditPrivate::copySqueezedText",
        "KJS::UString::Rep::hash",
        "KCModuleProxy::realModule",
        "KCategorizedView::visualRect",
        "KateLineLayout::textLine",
        "DOM::HTMLCollectionImpl::firstItem",
        "DOM::HTMLCollectionImpl::nextItem",
        "DOM::HTMLCollectionImpl::firstItem",
        "ImapResourceBase::settings",
    };

    return clazy::contains(files, qualifiedName);
}

void RequiredResults::VisitStmt(clang::Stmt *stm)
{
    auto compound = dyn_cast<CompoundStmt>(stm);
    if (!compound)
        return;

    for (auto child : compound->children()) {
        auto callExpr = dyn_cast<CXXMemberCallExpr>(child);
        if (!callExpr)
            continue;

        CXXMethodDecl *methodDecl = callExpr->getMethodDecl();
        if (!methodDecl || !methodDecl->isConst())
            continue;

        std::string methodName = methodDecl->getQualifiedNameAsString();
        if (shouldIgnoreMethod(methodName)) // Filter out some false positives
            continue;

        QualType qt = methodDecl->getReturnType();
        const Type *type = qt.getTypePtrOrNull();
        if (!type || type->isVoidType())
            continue;

        // Bail-out if any parameter is a non-const-ref or pointer bool bailout = false;
        bool bailout = false;
        for (auto paramVarDecl : Utils::functionParameters(methodDecl)) {
            QualType qt = paramVarDecl->getType();
            const Type *type = qt.getTypePtrOrNull();
            if (!type || type->isPointerType()) {
                bailout = true;
                break;
            }

            // qt.isConstQualified() not working !? TODO: Replace this string parsing when I figure it out
            if (type->isReferenceType() && !clazy::contains(qt.getAsString(), "const ")) {
                bailout = true;
                break;
            }
        }

        if (!bailout) {
            std::string error = std::string("Unused result of const member (") + methodName + ')';
            emitWarning(callExpr->getBeginLoc(), error);
        }
    }
}
