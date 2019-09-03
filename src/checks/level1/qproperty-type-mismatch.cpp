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
}

void QPropertyTypeMismatch::VisitDecl(clang::Decl *decl)
{
    if (auto method = dyn_cast<CXXMethodDecl>(decl))
        VisitMethod(*method);
    else if (auto field = dyn_cast<FieldDecl>(decl))
        VisitField(*field);
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

std::string QPropertyTypeMismatch::cleanupType(QualType type) {
    type = type.getNonReferenceType().getCanonicalType().getUnqualifiedType();
    //type.removeLocalCVRQualifiers(Qualifiers::CVRMask);

    std::string str = type.getAsString();
    if(str.compare(0, 6, "class ") == 0)
        str = str.substr(6);
    else if(str.compare(0, 7, "struct ") == 0)
        str = str.substr(7);

    str.erase(std::remove_if(str.begin(), str.end(), [] (char c) { return std::isspace(c); }), str.end());

    return str;
}

void QPropertyTypeMismatch::checkMethodAgainstProperty (const Property& prop, const CXXMethodDecl& method, const std::string& methodName){

    auto error_begin = [&] { return "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with "; };

    if(prop.read == methodName)
    {
        auto retTypeStr = cleanupType(method.getReturnType());
        if(prop.type != retTypeStr)
        {
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
            auto parmTypeStr = cleanupType(method.getParamDecl(0)->getType());
            if(prop.type != parmTypeStr)
            {
                emitWarning(&method, error_begin() + "method '" + methodName + "' with parameter of type '"+ parmTypeStr +"'");
            }
            break;
        }
        default:
            emitWarning(&method, error_begin() + "method '" + methodName + "' with too many parameters");
            break;
        }
    }
    else if(prop.notify == methodName)
    {
        switch(method.getNumParams())
        {
        case 0:
            // Should this case be ok ?
            // I don't think it is good practice to have signals of a property without
            // the property value in parameter, but afaik it's valid in Qt.
            emitWarning(&method, "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with signal '" + methodName + "' with no parameters");
            break;
        case 2:
        {
            auto param1TypeStr = cleanupType(method.getParamDecl(1)->getType());
            if(param1TypeStr.find("QPrivateSignal") == std::string::npos)
            {
                emitWarning(&method, error_begin() + "signal '" + methodName + "' with too many parameters" + param1TypeStr);
                break;
            }

            // We want to check the first parameter too :
            [[fallthrough]];
        }
        case 1:
        {
            auto param0TypeStr = cleanupType(method.getParamDecl(0)->getType());
            if(prop.type != param0TypeStr)
            {
                emitWarning(&method, error_begin() + "signal '" + methodName + "' with parameter of type '"+ param0TypeStr +"'");
            }
            break;
        }
        default:
        {
            emitWarning(&method, error_begin() + "signal '" + methodName + "' with too many parameters");
            break;
        }
        }
    }
}

void QPropertyTypeMismatch::checkFieldAgainstProperty (const Property& prop, const FieldDecl& field, const std::string& fieldName)
{
    if(prop.member && prop.name == fieldName)
    {
        auto typeStr = cleanupType(field.getType());
        if(prop.type != typeStr)
        {
            emitWarning(&field, "Q_PROPERTY '" + prop.name + "' of type '" + prop.type + "' is mismatched with member '" + fieldName + "' of type '"+ typeStr +"'");
        }
    }
}

void QPropertyTypeMismatch::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if(!ii)
        return;
    if(ii->getName() != "Q_PROPERTY")
        return;

    CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());

    string text = Lexer::getSourceText(crange, sm(), lo());
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

