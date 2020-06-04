/*
  This file is part of the clazy static checker.

    Copyright (C) 2019 Jean-Michaël Celerier <jeanmichael.celerier@gmail.com>

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

#include "qproperty-type-mismatch.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"

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

namespace clang {
class Decl;
class MacroInfo;
}  // namespace clang

using namespace clang;
using namespace std;


QPropertyTypeMismatch::QPropertyTypeMismatch(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enableVisitallTypeDefs();
}

void QPropertyTypeMismatch::VisitDecl(clang::Decl *decl)
{
    if (auto method = dyn_cast<CXXMethodDecl>(decl))
        VisitMethod(*method);
    else if (auto field = dyn_cast<FieldDecl>(decl))
        VisitField(*field);
    else if (auto typedefdecl = dyn_cast<TypedefNameDecl>(decl))
        VisitTypedef(typedefdecl);
}

void QPropertyTypeMismatch::VisitMethod(const clang::CXXMethodDecl & method)
{
    if (method.isThisDeclarationADefinition() && !method.hasInlineBody())
        return;

    const auto& theClass = method.getParent();
    const auto& classRange = theClass->getSourceRange();
    const auto& methodName = method.getNameInfo().getName().getAsString();

    for(const auto& prop : m_qproperties)
    {
        if(classRange.getBegin() < prop.loc && prop.loc < classRange.getEnd())
        {
            checkMethodAgainstProperty(prop, method, methodName);
        }
    }
}

void QPropertyTypeMismatch::VisitField(const FieldDecl & field)
{
    const auto& theClass = field.getParent();
    const auto& classRange = theClass->getSourceRange();
    const auto& methodName = field.getName().str();

    for(const auto& prop : m_qproperties)
    {
        if(classRange.getBegin() < prop.loc && prop.loc < classRange.getEnd())
        {
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
    str.erase(std::remove_if(str.begin(), str.end(), [] (char c) {
        return std::isspace(c);
    }), str.end());

    return str;
}

void QPropertyTypeMismatch::checkMethodAgainstProperty (const Property& prop, const CXXMethodDecl& method, const std::string& methodName){

    auto error_begin = [&] { return "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with "; };

    if (prop.read == methodName) {
        std::string retTypeStr;
        if (!typesMatch(prop.type, method.getReturnType(), retTypeStr)) {
            emitWarning(&method, error_begin() + "method '" + methodName + "' of return type '"+ retTypeStr +"'");
        }
    }
    else if(prop.write == methodName)
    {
        switch(method.getNumParams())
        {
        case 0:
            emitWarning(&method, error_begin() + "method '" + methodName + "' with no parameters");
            break;
        case 1:
        {
            std::string parmTypeStr;
            if (!typesMatch(prop.type, method.getParamDecl(0)->getType(), parmTypeStr))
                emitWarning(&method, error_begin() + "method '" + methodName + "' with parameter of type '"+ parmTypeStr +"'");
            break;
        }
        default:
            // Commented out: Too verbose and it's not a bug, maybe wrap with an option for the purists
            // emitWarning(&method, error_begin() + "method '" + methodName + "' with too many parameters");
            break;
        }
    }
    else if(prop.notify == methodName)
    {
        switch(method.getNumParams())
        {
        case 0:
            break;
        case 2:
        {
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
        case 1:
        {
            std::string param0TypeStr;
            if (!typesMatch(prop.type, method.getParamDecl(0)->getType(), param0TypeStr)) {
                const bool isPrivateSignal = param0TypeStr.find("QPrivateSignal") != std::string::npos;
                if (!isPrivateSignal)
                    emitWarning(&method, error_begin() + "signal '" + methodName + "' with parameter of type '"+ param0TypeStr +"'");
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }
}

void QPropertyTypeMismatch::checkFieldAgainstProperty (const Property& prop, const FieldDecl& field, const std::string& fieldName)
{
    if (prop.member && prop.name == fieldName) {
        std::string typeStr;
        if (!typesMatch(prop.type, field.getType(), typeStr))
            emitWarning(&field, "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with member '" + fieldName + "' of type '"+ typeStr +"'");
    }
}

bool QPropertyTypeMismatch::typesMatch(const string &type1, QualType type2Qt, std::string &type2Cleaned) const
{
    type2Cleaned = cleanupType(type2Qt);
    if (type1 == type2Cleaned)
        return true;

    // Maybe it's a typedef
    auto it = m_typedefMap.find(type1);
    if (it != m_typedefMap.cend()) {
        return it->second == type2Qt || cleanupType(it->second) == type2Cleaned;
    }

    // Maybe the difference is just the scope, if yes then don't warn. We already have a check for complaining about lack of scope
    type2Cleaned = cleanupType(type2Qt, /*unscopped=*/ true);
    if (type1 == type2Cleaned)
        return true;

    return false;
}

void QPropertyTypeMismatch::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if(!ii)
        return;
    if(ii->getName() != "Q_PROPERTY")
        return;

    CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());

    string text = static_cast<string>(Lexer::getSourceText(crange, sm(), lo()));
    if (!text.empty() && text.back() == ')')
        text.pop_back();

    std::vector<std::string> split = clazy::splitString(text, ' ');
    if(split.size() < 2)
        return;

    Property p;
    p.loc = range.getBegin();

    // Handle type
    clazy::rtrim(split[0]);
    p.type = split[0];
    if(p.type.find("Q_PROPERTY(") == 0)
        p.type = p.type.substr(11);

    // Handle name
    clazy::rtrim(split[1]);
    p.name = split[1];

    // FIXME: This is getting hairy, better use regexps
    for (unsigned int i = 0; i < p.name.size(); ++i) {
        if (p.name[i] == '*') {
            p.type += '*';
        } else {
            break;
        }
    }

    p.name.erase(std::remove(p.name.begin(), p.name.end(), '*'), p.name.end());

    // Handle Q_PROPERTY functions
    enum {
        None, Read, Write, Notify
    } next = None;

    for (std::string &token : split) {
        clazy::rtrim(/*by-ref*/token);
        switch(next)
        {
        case None:
        {
            if (token == "READ") {
                next = Read;
                continue;
            }
            else if (token == "WRITE") {
                next = Write;
                continue;
            }
            else if (token == "NOTIFY") {
                next = Notify;
                continue;
            }
            else if (token == "MEMBER") {
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

