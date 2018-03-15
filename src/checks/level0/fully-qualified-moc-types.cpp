/*
  This file is part of the clazy static checker.

  Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "fully-qualified-moc-types.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


FullyQualifiedMocTypes::FullyQualifiedMocTypes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
    enablePreProcessorCallbacks();
}

void FullyQualifiedMocTypes::VisitDecl(clang::Decl *decl)
{
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!method)
        return;

    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager)
        return;

    if (handleQ_PROPERTY(method))
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody())
        return;

    QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
    if (qst != QtAccessSpecifier_Signal && qst != QtAccessSpecifier_Slot && qst != QtAccessSpecifier_Invokable)
        return;

    for (auto param : method->parameters()) {
        QualType t = TypeUtils::pointeeQualType(param->getType());
        if (!t.isNull()) {
            std::string typeName = clazy::name(t, lo(), /*asWritten=*/ true);
            if (typeName == "QPrivateSignal")
                continue;

            std::string qualifiedTypeName = clazy::name(t, lo(), /*asWritten=*/ false);

            if (typeName != qualifiedTypeName) {
                emitWarning(method, string(accessSpecifierManager->qtAccessSpecifierTypeStr(qst)) + " arguments need to be fully-qualified (" + qualifiedTypeName + " instead of " + typeName + ")");
            }
        }
    }
}

bool FullyQualifiedMocTypes::isGadget(CXXRecordDecl *record) const
{
    SourceLocation startLoc = record->getLocStart();
    for (const SourceLocation &loc : m_qgadgetMacroLocations) {
        if (sm().getFileID(loc) != sm().getFileID(startLoc))
            continue; // Different file

        if (sm().isBeforeInSLocAddrSpace(startLoc, loc) && sm().isBeforeInSLocAddrSpace(loc, record->getLocEnd()))
            return true; // We found a Q_GADGET after start and before end, it's ours.
    }
    return false;
}

bool FullyQualifiedMocTypes::handleQ_PROPERTY(CXXMethodDecl *method)
{
    if (clazy::name(method) != "qt_static_metacall" || !method->hasBody() || method->getDefinition() != method)
        return false;
    /**
     * Basically diffed a .moc file with and without a namespaced property,
     * the difference is one reinterpret_cast, under an if (_c == QMetaObject::ReadProperty), so
     * that's what we cheak.
     *
     * The AST doesn't have the Q_PROPERTIES as they expand to nothing, so we have
     * to do it this way.
     */

    auto ifs = clazy::getStatements<IfStmt>(method->getBody());

    for (auto iff : ifs) {
        auto bo = dyn_cast<BinaryOperator>(iff->getCond());
        if (!bo)
            continue;

        auto enumRefs = clazy::getStatements<DeclRefExpr>(bo->getRHS());
        if (enumRefs.size() == 1) {
            auto enumerator = dyn_cast<EnumConstantDecl>(enumRefs.at(0)->getDecl());
            if (enumerator && enumerator->getName() == "ReadProperty") {
                auto reinterprets = clazy::getStatements<CXXReinterpretCastExpr>(iff);
                for (auto reinterpret : reinterprets) {
                    QualType qt = TypeUtils::pointeeQualType(reinterpret->getTypeAsWritten());
                    auto record = qt->getAsCXXRecordDecl();
                    if (!record || !isGadget(record))
                        continue;

                    string nameAsWritten = clazy::name(qt, lo(), /*asWritten=*/ true);
                    string fullyQualifiedName = clazy::name(qt, lo(), /*asWritten=*/ false);
                    if (nameAsWritten != fullyQualifiedName) {
                        // warn in the cxxrecorddecl, since we don't want to warn in the .moc files.
                        // Ideally we would do some cross checking with the Q_PROPERTIES, but that's not in the AST
                        emitWarning(method->getParent()->getLocStart(), "Q_PROPERTY of type " + nameAsWritten + " should use full qualification (" + fullyQualifiedName + ")");
                    }
                }
                return true;
            }
        }
    }

    return true; // true, so processing doesn't continue, it's a qt_static_metacall, nothing interesting here unless the properties above
}

void FullyQualifiedMocTypes::VisitMacroExpands(const clang::Token &MacroNameTok,
                                               const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && ii->getName() == "Q_GADGET")
        registerQ_GADGET(range.getBegin());
}

void FullyQualifiedMocTypes::registerQ_GADGET(SourceLocation loc)
{
    m_qgadgetMacroLocations.push_back(loc);
}
