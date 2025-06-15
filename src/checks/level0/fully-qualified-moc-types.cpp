/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fully-qualified-moc-types.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

FullyQualifiedMocTypes::FullyQualifiedMocTypes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void FullyQualifiedMocTypes::VisitDecl(clang::Decl *decl)
{
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!method) {
        return;
    }

    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager) {
        return;
    }

    if (handleQ_PROPERTY(method)) {
        return;
    }

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) {
        return;
    }

    QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
    if (qst != QtAccessSpecifier_Signal && qst != QtAccessSpecifier_Slot && qst != QtAccessSpecifier_Invokable) {
        return;
    }

    std::string qualifiedTypeName;
    std::string typeName;
    for (auto *param : method->parameters()) {
        QualType t = clazy::pointeeQualType(param->getType());
        if (!typeIsFullyQualified(t, /*by-ref*/ qualifiedTypeName, /*by-ref*/ typeName)) {
            SourceRange fixitRange = param->getTypeSourceInfo()->getTypeLoc().getSourceRange();
            // We don't want to include the & or * characters for the fixit range
            if (param->getType()->isReferenceType() || param->getType()->isPointerType()) {
                fixitRange = SourceRange(fixitRange.getBegin(), fixitRange.getEnd().getLocWithOffset(-1));
            }
            std::vector fixits{FixItHint::CreateReplacement(fixitRange, qualifiedTypeName)};
            std::string warning = accessSpecifierManager->qtAccessSpecifierTypeStr(qst).str() + " arguments need to be fully-qualified";
            emitWarning(param->getTypeSpecStartLoc(), warning, fixits);
        }
    }

    if (qst == QtAccessSpecifier_Slot || qst == QtAccessSpecifier_Invokable) {
        QualType returnT = clazy::pointeeQualType(method->getReturnType());
        if (!typeIsFullyQualified(returnT, /*by-ref*/ qualifiedTypeName, /*by-ref*/ typeName)) {
            SourceRange returnTypeSourceRange = method->getReturnTypeSourceRange();
            // We don't want to include the & or * characters for the fixit range
            if (method->getReturnType()->isReferenceType() || method->getReturnType()->isPointerType()) {
                returnTypeSourceRange = SourceRange(returnTypeSourceRange.getBegin(), returnTypeSourceRange.getEnd().getLocWithOffset(-1));
            }
            std::string warning = accessSpecifierManager->qtAccessSpecifierTypeStr(qst).str() + " return types need to be fully-qualified";
            std::vector fixits{FixItHint::CreateReplacement(returnTypeSourceRange, qualifiedTypeName)};
            emitWarning(returnTypeSourceRange.getBegin(), warning, fixits);
        }
    }
}

std::string FullyQualifiedMocTypes::getQualifiedNameOfType(const Type *ptr, bool checkElabType) const
{
    if (auto *elabType = dyn_cast<ElaboratedType>(ptr); elabType && checkElabType) {
        if (auto *specType = dyn_cast<TemplateSpecializationType>(elabType->getNamedType().getTypePtrOrNull()); specType && !ptr->getAs<TypedefType>()) {
            return resolveTemplateType(specType, false);
        }
        if (elabType->isEnumeralType() && elabType->getAs<EnumType>()) {
            return elabType->getAs<EnumType>()->getDecl()->getQualifiedNameAsString();
        }
    }
    if (auto *typedefDecl = ptr->getAs<TypedefType>(); typedefDecl && typedefDecl->getDecl()) {
        return typedefDecl->getDecl()->getQualifiedNameAsString();
    } else if (auto templateSpec = ptr->getAs<TemplateSpecializationType>()) {
        // In case one uses a typedef with generics, like QVector<QString> in Qt6
        // The docs indicate getAsTemplateDecl might be null - so be prepared for that
        if (auto *decl = templateSpec->getTemplateName().getAsTemplateDecl()) {
            return decl->getQualifiedNameAsString();
        }
    } else if (auto recordDecl = ptr->getAsRecordDecl()) {
        return recordDecl->getQualifiedNameAsString();
    }
    return QualType::getFromOpaquePtr(ptr).getAsString(lo());
}

// In Qt5, the type would contain lots of unneeded parameters: QDBusPendingReply<bool, void, void, void, void, void, void, void>
// Thus we need to do all of the shenanigans below
// specType->getCanonicalTypeInternal().getAsString(m_astContext.getPrintingPolicy())
std::string FullyQualifiedMocTypes::resolveTemplateType(const clang::TemplateSpecializationType *ptr, bool checkElabType) const
{
    std::string str = getQualifiedNameOfType(ptr, checkElabType);
    str += "<";
    bool firstArg = true;
    for (auto arg : ptr->template_arguments()) { // We reconstruct the type with the explicitly specified template params
        if (!firstArg) {
            str += ", ";
        }
        firstArg = false;
        if (arg.getKind() == TemplateArgument::Expression) {
            // std::bitset<int(8)>, here int(8) is an expression!
            llvm::raw_string_ostream outputStream(str);
            arg.print(astContext()->getPrintingPolicy(), outputStream, true);
        } else {
            QualType argType = arg.getAsType();
            if (argType.isConstQualified()) {
                str += "const ";
            }
            str += getQualifiedNameOfType(argType.getTypePtr());
        }
    }
    str += ">";
    return str;
}

bool FullyQualifiedMocTypes::typeIsFullyQualified(QualType t, std::string &qualifiedTypeName, std::string &typeName) const
{
    qualifiedTypeName.clear();
    typeName.clear();

    if (auto *ptr = t.getTypePtrOrNull(); ptr && (ptr->isRecordType() || ptr->isEnumeralType())) {
        typeName = clazy::name(t.getUnqualifiedType(), lo(), /*asWritten=*/true); // Ignore qualifiers like const here
        if (typeName == "QPrivateSignal") {
            return true;
        }

        if (auto specType = ptr->getAs<TemplateSpecializationType>(); specType && !ptr->getAs<TypedefType>()) {
            qualifiedTypeName = resolveTemplateType(specType);
        } else if (auto recordDecl = ptr->getAsRecordDecl(); recordDecl && recordDecl->isInAnonymousNamespace()) {
            return true;
        } else {
            qualifiedTypeName = getQualifiedNameOfType(ptr);
        }
        return qualifiedTypeName.empty() || typeName == qualifiedTypeName;
    }
    return true;
}

bool FullyQualifiedMocTypes::isGadget(CXXRecordDecl *record) const
{
    SourceLocation startLoc = record->getBeginLoc();
    for (const SourceLocation &loc : m_qgadgetMacroLocations) {
        if (sm().getFileID(loc) != sm().getFileID(startLoc)) {
            continue; // Different file
        }

        if (sm().isBeforeInSLocAddrSpace(startLoc, loc) && sm().isBeforeInSLocAddrSpace(loc, record->getEndLoc())) {
            return true; // We found a Q_GADGET after start and before end, it's ours.
        }
    }
    return false;
}

bool FullyQualifiedMocTypes::handleQ_PROPERTY(CXXMethodDecl *method)
{
    if (clazy::name(method) != "qt_static_metacall" || !method->hasBody() || method->getDefinition() != method) {
        return false;
    }
    /**
     * Basically diffed a .moc file with and without a namespaced property,
     * the difference is one reinterpret_cast, under an if (_c == QMetaObject::ReadProperty), so
     * that's what we cheak.
     *
     * The AST doesn't have the Q_PROPERTIES as they expand to nothing, so we have
     * to do it this way.
     */

    auto ifs = clazy::getStatements<IfStmt>(method->getBody());

    for (auto *iff : ifs) {
        auto *bo = dyn_cast<BinaryOperator>(iff->getCond());
        if (!bo) {
            continue;
        }

        auto enumRefs = clazy::getStatements<DeclRefExpr>(bo->getRHS());
        if (enumRefs.size() == 1) {
            auto *enumerator = dyn_cast<EnumConstantDecl>(enumRefs.at(0)->getDecl());
            if (enumerator && clazy::name(enumerator) == "ReadProperty") {
                auto switches = clazy::getStatements<SwitchStmt>(iff); // we only want the reinterpret_casts that are inside switches
                for (auto *s : switches) {
                    auto reinterprets = clazy::getStatements<CXXReinterpretCastExpr>(s);
                    for (auto *reinterpret : reinterprets) {
                        QualType qt = clazy::pointeeQualType(reinterpret->getTypeAsWritten());
                        if (auto *record = qt->getAsCXXRecordDecl(); !record || !isGadget(record)) {
                            continue;
                        }

                        std::string nameAsWritten;
                        std::string qualifiedName;
                        if (!typeIsFullyQualified(qt, qualifiedName, nameAsWritten)) {
                            // warn in the cxxrecorddecl, since we don't want to warn in the .moc files.
                            // Ideally we would do some cross checking with the Q_PROPERTIES, but that's not in the AST
                            emitWarning(method->getParent()->getBeginLoc(),
                                        "Q_PROPERTY of type " + nameAsWritten + " should use full qualification (" + qualifiedName + ")");
                        }
                    }
                }
                return true;
            }
        }
    }

    return true; // true, so processing doesn't continue, it's a qt_static_metacall, nothing interesting here unless the properties above
}

void FullyQualifiedMocTypes::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && ii->getName() == "Q_GADGET") {
        registerQ_GADGET(range.getBegin());
    }
}

void FullyQualifiedMocTypes::registerQ_GADGET(SourceLocation loc)
{
    m_qgadgetMacroLocations.push_back(loc);
}
