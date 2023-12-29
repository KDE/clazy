/*
    SPDX-FileCopyrightText: 2019 Jean-Michaël Celerier <jeanmichael.celerier@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qproperty-type-mismatch.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
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

#include <algorithm>
#include <cctype>
#include <string_view>

namespace clang
{
class Decl;
class MacroInfo;
} // namespace clang

using namespace clang;

QPropertyTypeMismatch::QPropertyTypeMismatch(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enableVisitallTypeDefs();
}

void QPropertyTypeMismatch::VisitDecl(clang::Decl *decl)
{
    if (auto *method = dyn_cast<CXXMethodDecl>(decl)) {
        VisitMethod(*method);
    } else if (auto *field = dyn_cast<FieldDecl>(decl)) {
        VisitField(*field);
    } else if (auto *typedefdecl = dyn_cast<TypedefNameDecl>(decl)) {
        VisitTypedef(typedefdecl);
    }
}

void QPropertyTypeMismatch::VisitMethod(const clang::CXXMethodDecl &method)
{
    if (method.isThisDeclarationADefinition() && !method.hasInlineBody()) {
        return;
    }

    const auto &theClass = method.getParent();
    const auto &classRange = theClass->getSourceRange();
    const auto &methodName = method.getNameInfo().getName().getAsString();

    for (const auto &prop : m_qproperties) {
        if (classRange.getBegin() < prop.loc && prop.loc < classRange.getEnd()) {
            checkMethodAgainstProperty(prop, method, methodName);
        }
    }
}

void QPropertyTypeMismatch::VisitField(const FieldDecl &field)
{
    const auto &theClass = field.getParent();
    const auto &classRange = theClass->getSourceRange();
    const auto &methodName = field.getName().str();

    for (const auto &prop : m_qproperties) {
        if (classRange.getBegin() < prop.loc && prop.loc < classRange.getEnd()) {
            checkFieldAgainstProperty(prop, field, methodName);
        }
    }
}

void QPropertyTypeMismatch::VisitTypedef(const clang::TypedefNameDecl *td)
{
    // Since when processing Q_PROPERTY we're at the pre-processor stage we don't have access
    // to the Qualtypes, so catch any typedefs here
    QualType underlyingType = td->getUnderlyingType();
    m_typedefMap[td->getQualifiedNameAsString()] = underlyingType;
    m_typedefMap[td->getNameAsString()] = underlyingType; // It might be written unqualified in the Q_PROPERTY

    // FIXME: All the above is a bit flaky, as we don't know the actual namespace when the type is written without namespace in Q_PROPERTY
    // Proper solution would be to process the .moc instead of doing text manipulation with the macros we receive
}

std::string QPropertyTypeMismatch::cleanupType(QualType type, bool unscoped) const
{
    type = type.getNonReferenceType().getCanonicalType().getUnqualifiedType();

    PrintingPolicy po(lo());
    po.SuppressTagKeyword = true;
    po.SuppressScope = unscoped;

    std::string str = type.getAsString(po);
    str.erase(std::remove_if(str.begin(),
                             str.end(),
                             [](char c) {
                                 return std::isspace(c);
                             }),
              str.end());

    return str;
}

void QPropertyTypeMismatch::checkMethodAgainstProperty(const Property &prop, const CXXMethodDecl &method, const std::string &methodName)
{
    auto error_begin = [&] {
        return "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with ";
    };

    if (prop.read == methodName) {
        std::string retTypeStr;
        if (!typesMatch(prop.type, method.getReturnType(), retTypeStr)) {
            emitWarning(&method, error_begin() + "method '" + methodName + "' of return type '" + retTypeStr + "'");
        }
    } else if (prop.write == methodName) {
        switch (method.getNumParams()) {
        case 0:
            emitWarning(&method, error_begin() + "method '" + methodName + "' with no parameters");
            break;
        case 1: {
            std::string parmTypeStr;
            if (!typesMatch(prop.type, method.getParamDecl(0)->getType(), parmTypeStr)) {
                emitWarning(&method, error_begin() + "method '" + methodName + "' with parameter of type '" + parmTypeStr + "'");
            }
            break;
        }
        default:
            // Commented out: Too verbose and it's not a bug, maybe wrap with an option for the purists
            // emitWarning(&method, error_begin() + "method '" + methodName + "' with too many parameters");
            break;
        }
    } else if (prop.notify == methodName) {
        switch (method.getNumParams()) {
        case 0:
            break;
        case 2: {
            /*
             // Commented out: Too verbose and it's not a bug, maybe wrap with an option for the purists
            auto param1TypeStr = cleanupType(method.getParamDecl(1)->getType());
            if(param1TypeStr.find("QPrivateSignal") == std::string::npos)
            {
                emitWarning(&method, error_begin() + "signal '" + methodName + "' with too many parameters" + param1TypeStr);
                break;
            }

            // We want to check the first parameter too :
            [[fallthrough]];*/
        }
        case 1: {
            std::string param0TypeStr;
            if (!typesMatch(prop.type, method.getParamDecl(0)->getType(), param0TypeStr)) {
                const bool isPrivateSignal = param0TypeStr.find("QPrivateSignal") != std::string::npos;
                if (!isPrivateSignal) {
                    emitWarning(&method, error_begin() + "signal '" + methodName + "' with parameter of type '" + param0TypeStr + "'");
                }
            }
            break;
        }
        default: {
            break;
        }
        }
    }
}

void QPropertyTypeMismatch::checkFieldAgainstProperty(const Property &prop, const FieldDecl &field, const std::string &fieldName)
{
    if (prop.member && prop.name == fieldName) {
        std::string typeStr;
        if (!typesMatch(prop.type, field.getType(), typeStr)) {
            emitWarning(&field,
                        "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with member '" + fieldName + "' of type '" + typeStr + "'");
        }
    }
}

bool QPropertyTypeMismatch::typesMatch(const std::string &type1, QualType type2Qt, std::string &type2Cleaned) const
{
    type2Cleaned = cleanupType(type2Qt);
    if (type1 == type2Cleaned) {
        return true;
    }

    // Maybe it's a typedef
    auto it = m_typedefMap.find(type1);
    if (it != m_typedefMap.cend()) {
        return it->second == type2Qt || cleanupType(it->second) == type2Cleaned;
    }

    // Maybe the difference is just the scope, if yes then don't warn. We already have a check for complaining about lack of scope
    type2Cleaned = cleanupType(type2Qt, /*unscopped=*/true);
    return type1 == type2Cleaned;
}

void QPropertyTypeMismatch::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii) {
        return;
    }
    if (ii->getName() != "Q_PROPERTY") {
        return;
    }

    CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());

    std::string text = static_cast<std::string>(Lexer::getSourceText(crange, sm(), lo()));
    using namespace std::string_view_literals;
    constexpr std::string_view q_property_brace = "Q_PROPERTY("sv;
    if (clazy::startsWith(text, q_property_brace)) {
        text = text.substr(q_property_brace.size());
    }

    if (!text.empty() && text.back() == ')') {
        text.pop_back();
    }

    std::vector<std::string_view> split = clazy::splitStringBySpaces(text);
    if (split.size() < 2) {
        return;
    }

    Property p;
    p.loc = range.getBegin();

    std::size_t splitIndex = 0;

    // Handle type (type string and any following modifiers)
    const auto isModifier = [](std::string_view str) {
        return str == "*"sv || str == "&"sv;
    };

    for (; isModifier(split[splitIndex]) || p.type.empty(); ++splitIndex) {
        p.type += split[splitIndex];
    }

    // Handle name
    p.name = split[splitIndex];

    std::size_t actualNameStartPos = 0;
    // FIXME: This is getting hairy, better use regexps
    for (unsigned int i = 0; i < p.name.size(); ++i) {
        if (p.name[i] == '*' || p.name[i] == '&') {
            p.type += p.name[i];
            ++actualNameStartPos;
        } else {
            break;
        }
    }

    if (actualNameStartPos) {
        p.name.erase(0, actualNameStartPos);
    }

    // Handle Q_PROPERTY functions
    enum { None, Read, Write, Notify } next = None;

    for (std::string_view &token : split) {
        switch (next) {
        case None: {
            if (token == "READ"sv) {
                next = Read;
                continue;
            } else if (token == "WRITE"sv) {
                next = Write;
                continue;
            } else if (token == "NOTIFY"sv) {
                next = Notify;
                continue;
            } else if (token == "MEMBER"sv) {
                p.member = true;
                break;
            }
            break;
        }
        case Read:
            p.read = token;
            break;
        case Write:
            p.write = token;
            break;
        case Notify:
            p.notify = token;
            break;
        }

        next = None;
    }

    m_qproperties.push_back(std::move(p));
}
