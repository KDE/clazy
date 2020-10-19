/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

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

#include "qt6-deprecated-api-fixes.h"
#include "ContextUtils.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "StringUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/Lex/Lexer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;

Qt6DeprecatedAPIFixes::Qt6DeprecatedAPIFixes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

std::string Qt6DeprecatedAPIFixes::findPathArgument(clang::Stmt *stmt, bool ancestorIsCondition, int ancestorConditionChildNumber)
{
    std::string replacement;
    Stmt *current_stmt = stmt;

    int i = 0;

    for (auto it = current_stmt->child_begin() ; it !=current_stmt->child_end() ; it++) {

        Stmt *child = *it;
        ConditionalOperator *parent_condOp = dyn_cast<ConditionalOperator>(current_stmt);
        ConditionalOperator *child_condOp = dyn_cast<ConditionalOperator>(child);

        /*
        For cases like: dir = cond ? "path1" : "path2";
        simplified AST after the operand= look like this:
        CXXBindTemporaryExpr
            |   `-CXXConstructExpr
            |     `-ConditionalOperator.......................... The ancestor that is a conditional operator
            |       |-ImplicitCastExpr 'bool'........................ ancestorConditionChildNumber == 0
            |       | `-DeclRefExpr  'cond' 'bool'
            |       |-ImplicitCastExpr 'const char *'................ ancestorConditionChildNumber == 1
            |       | `-StringLiteral 'const char []' "path1"    => Need to know it has ConditionalOperator has ancestor
            |       |                                            => Need to know it comes from the second children of this ancestor
            |       |                                            => to put the ':' between the two StringLiteral
            |       `-ImplicitCastExpr 'const char *'................ ancestorConditionChildNumber == 2
            |         `-StringLiteral 'const char []' "path2"

        */
        if (parent_condOp) {
            ancestorIsCondition = true;
            ancestorConditionChildNumber = i;
            if (ancestorConditionChildNumber == 2)
                replacement += ":";
        }

        // to handle nested condition
        if (child_condOp && ancestorIsCondition) {
            replacement += "(";
        }

        replacement += findPathArgument(child, ancestorIsCondition, ancestorConditionChildNumber);

        DeclRefExpr *child_declRefExp = dyn_cast<DeclRefExpr>(child);
        CXXBoolLiteralExpr *child_boolLitExp = dyn_cast<CXXBoolLiteralExpr>(child);
        StringLiteral *child_stringliteral = dyn_cast<StringLiteral>(child);

        if (child_stringliteral) {
            replacement += "\"";
            replacement += child_stringliteral->getString();
            replacement += "\"";
        } else if (child_boolLitExp) {
            replacement = child_boolLitExp->getValue() ? "true" : "false";
                    replacement += " ? ";
        } else if (child_declRefExp) {
            if (ancestorIsCondition && ancestorConditionChildNumber == 0
                    && child_declRefExp->getType().getAsString() == "_Bool") {
                replacement += child_declRefExp->getNameInfo().getAsString();
                replacement += " ? ";
            } else {
                //assuming that the variable is compatible with setPath function.
                // if the code was compiling with dir = variable, should be ok to write dir.setPath(variable)
                replacement += child_declRefExp->getNameInfo().getAsString();
            }
       } else if (child_condOp && ancestorIsCondition) {
            replacement += ")";
       }

        i++;
    }
    return replacement;
}

static std::set<std::string> qProcessDeprecatedFunctions = {"start", "execute", "startDetached"};

void replacementForQProcess(string functionName, string &message, string &replacement) {
    message = "call function QProcess::";
    message += functionName;
    message += "(). Use function QProcess::";
    message += functionName;
    message += "Command() instead";

    replacement = functionName;
    replacement += "Command";
}

void replacementForQSignalMapper(clang::MemberExpr * membExpr, string &message, string &replacement) {

    auto declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    string paramType;
    for (auto param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString();
    }

    string functionNameExtention;
    string paramTypeCor;
    if (paramType == "int") {
        functionNameExtention = "Int";
        paramTypeCor = "int";
    } else if (paramType == "const class QString &") {
        functionNameExtention = "String";
        paramTypeCor = "const QString &";
    } else if (paramType == "class QWidget *") {
        functionNameExtention = "Widget";
        paramTypeCor = "QWidget *";
    } else if (paramType == "class QObject *") {
        functionNameExtention = "Object";
        paramTypeCor = "QObject *";
    }

    message = "call function QSignalMapper::mapped(";
    message += paramTypeCor;
    message += "). Use function QSignalMapper::mapped";
    message += functionNameExtention;
    message += "(";
    message += paramTypeCor;
    message += ") instead.";

    replacement = "mapped";
    replacement += functionNameExtention;
}

void replacementForQResource(string functionName, string &message, string &replacement) {
    message = "call function QRessource::isCompressed(). Use function QProcess::compressionAlgorithm() instead.";
    replacement = "compressionAlgorithm";
}

static std::set<std::string> qSetDeprecatedOperators = {"operator--", "operator+", "operator-", "operator+=", "operator-="};
static std::set<std::string> qSetDeprecatedFunctions = {"rbegin", "rend", "crbegin", "crend", "hasPrevious", "previous",
                                                       "peekPrevious", "findPrevious"};
static std::set<std::string> qHashDeprecatedFunctions = {"hasPrevious", "previous",
                                                       "peekPrevious", "findPrevious"};

bool isQSetDepreprecatedOperator(string functionName, string contextName, string &message)
{
    if (qSetDeprecatedOperators.find(functionName) == qSetDeprecatedOperators.end())
        return false;
    if ((clazy::startsWith(contextName, "QSet<") || clazy::startsWith(contextName, "QHash<")) &&
            clazy::endsWith(contextName, "iterator")) {

        if (clazy::startsWith(contextName, "QSet<"))
            message = "QSet iterator categories changed from bidirectional to forward. Please port your code manually";
        else
            message = "QHash iterator categories changed from bidirectional to forward. Please port your code manually";

        return true;
    }
    return false;
}

static std::set<std::string> qMapFunctions = {"insertMulti", "uniqueKeys", "values", "unite"};

static std::set<std::string> qTextStreamFunctions = {
    "bin", "oct", "dec", "hex", "showbase", "forcesign", "forcepoint", "noshowbase", "noforcesign", "noforcepoint",
    "uppercasebase", "uppercasedigits", "lowercasebase", "lowercasedigits", "fixed", "scientific", "left", "right",
    "center", "endl", "flush", "reset", "bom", "ws"};

void replacementForQTextStreamFunctions(string functionName, string &message, string &replacement, bool explicitQtNamespace) {
    if (qTextStreamFunctions.find(functionName) == qTextStreamFunctions.end())
        return;
    message = "call function QTextStream::";
    message += functionName;
    message += ". Use function Qt::";
    message += functionName;
    message += " instead";

    if (!explicitQtNamespace)
        replacement = "Qt::";
    replacement += functionName;
}

void replacementForQStringSplitBehavior(string functionName, string &message, string &replacement, bool explicitQtNamespace) {
    message = "Use Qt::SplitBehavior variant instead";
    if (!explicitQtNamespace)
        replacement = "Qt::";
    replacement += functionName;
}

void Qt6DeprecatedAPIFixes::VisitStmt(clang::Stmt *stmt)
{
    CXXOperatorCallExpr *oppCallExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    DeclRefExpr *declRefExp = dyn_cast<DeclRefExpr>(stmt);
    MemberExpr *membExpr = dyn_cast<MemberExpr>(stmt);

    SourceLocation warningLocation;
    std::string replacement;
    std::string message;
    SourceRange fixitRange;

    vector<FixItHint> fixits;

    if (oppCallExpr) {
        // dir = "foo" case
        if ( !clazy::isOfClass(oppCallExpr, "QDir") )
            return;

        // only interested in '=' operator
        Stmt *child = clazy::childAt(stmt, 0);
        while (child) {
            DeclRefExpr *decl = dyn_cast<DeclRefExpr>(child);
            if ( !decl ) {
                child = clazy::childAt(child, 0);
                continue;
            }
            if ( decl->getNameInfo().getAsString() == "operator=" ) {
                warningLocation = decl->getLocation();
                break;
            }
            child = clazy::childAt(child, 0);
        }

        // get the name of the QDir variable from child2 value
        child = clazy::childAt(stmt, 1);
        if (!child)
            return;
        DeclRefExpr *declb = dyn_cast<DeclRefExpr>(child);
        if ( !declb )
            return;
        message = " function setPath() has to be used in Qt6";
        // need to make sure there is no macro
        for (auto macro_pos : m_listingMacroExpand) {
            if (m_sm.isPointWithin(macro_pos, clazy::getLocStart(stmt), clazy::getLocEnd(stmt))) {
               emitWarning(warningLocation, message, fixits);
               return;
            }
        }
        replacement = declb->getNameInfo().getAsString();
        replacement += ".setPath(";
        replacement += findPathArgument(clazy::childAt(stmt, 2));
        replacement += ");";

        fixitRange = stmt->getSourceRange();

    } else if (declRefExp) {

        warningLocation = declRefExp->getBeginLoc();
        auto decl = declRefExp->getDecl();
        if (!decl)
            return;
        auto *declContext = declRefExp->getDecl()->getDeclContext();
        if (!declContext)
            return;
        string functionName = declRefExp->getNameInfo().getAsString();
        string enclosingNameSpace;
        auto *enclNsContext = declContext->getEnclosingNamespaceContext();
        if (enclNsContext) {
            if (auto *ns = dyn_cast<NamespaceDecl>(declContext->getEnclosingNamespaceContext()))
                enclosingNameSpace = ns->getNameAsString();
        }

        // To catch QDir and QSet
        string contextName;
        if (declContext) {
            if (clang::isa<clang::CXXRecordDecl>(declContext)) {
                clang::CXXRecordDecl *recordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(declContext);
                contextName = recordDecl->getQualifiedNameAsString();
            }
        }

        if (isQSetDepreprecatedOperator(functionName, contextName, message)) {
            emitWarning(warningLocation, message, fixits);
            return;
        }
        if (functionName == "addResourceSearchPath" && contextName == "QDir") {
            message = "call function QDir::addResourceSearchPath(). Use function QDir::addSearchPath() with prefix instead";
            emitWarning(warningLocation, message, fixits);
            return;
        }

        if (functionName != "MatchRegExp" && enclosingNameSpace != "QTextStreamFunctions" &&
            functionName != "KeepEmptyParts" && functionName != "SkipEmptyParts")
            return;

        // To catch enum Qt::MatchFlag and enum QString::SplitBehavior
        string declType;
        declType = decl->getType().getAsString();
        // Get out of this DeclRefExp to catch the potential Qt namespace surrounding it
        bool isQtNamespaceExplicit = false;
        DeclContext *newcontext =  clazy::contextForDecl(m_context->lastDecl);
        while (newcontext) {
            if (!newcontext)
                break;
            if (clang::isa<NamespaceDecl>(newcontext)) {
                clang::NamespaceDecl *namesdecl = dyn_cast<clang::NamespaceDecl>(newcontext);
                if (namesdecl->getNameAsString() == "Qt")
                    isQtNamespaceExplicit = true;
             }
            newcontext = newcontext->getParent();
        }

        if (enclosingNameSpace == "QTextStreamFunctions") {
            replacementForQTextStreamFunctions(functionName, message, replacement, isQtNamespaceExplicit);
        } else if ((functionName == "KeepEmptyParts" || functionName == "SkipEmptyParts") &&
                   declType == "enum QString::SplitBehavior") {
            replacementForQStringSplitBehavior(functionName, message, replacement, isQtNamespaceExplicit);
        } else if (functionName == "MatchRegExp" && declType == "enum Qt::MatchFlag") {
            message = "call Qt::MatchRegExp. Use Qt::MatchRegularExpression instead.";
            if (isQtNamespaceExplicit)
                replacement = "MatchRegularExpression";
            else
                replacement = "Qt::MatchRegularExpression";
        }else {
            return;
        }
        fixitRange = declRefExp->getSourceRange();

    }else if (membExpr) {
        Stmt *child = clazy::childAt(stmt, 0);
        DeclRefExpr *decl = dyn_cast<DeclRefExpr>(child);
        while (child) {
            if (decl)
                break;
            child = clazy::childAt(child, 0);
            decl = dyn_cast<DeclRefExpr>(child);
        }

        if (!decl)
            return;
        string functionName = membExpr->getMemberNameInfo().getAsString();
        string className = decl->getType().getAsString();
        warningLocation = membExpr->getEndLoc();

        if (clazy::startsWith(className, "QMap<") && qMapFunctions.find(functionName) != qMapFunctions.end()) {
            message = "Use QMultiMap for maps storing multiple values with the same key.";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QHash<") && qMapFunctions.find(functionName) != qMapFunctions.end()) {
          // the name of the deprecated function are the same
            message = "Use QMultiHash for maps storing multiple values with the same key.";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "class QDir") && functionName == "addResourceSearchPath") {
            message = "call function QDir::addResourceSearchPath(). Use function QDir::addSearchPath() with prefix instead";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "class QProcess") &&
                   qProcessDeprecatedFunctions.find(functionName) != qProcessDeprecatedFunctions.end()) {
            replacementForQProcess(functionName, message, replacement);
        } else if (clazy::startsWith(className, "class QResource") && functionName == "isCompressed") {
            replacementForQResource(functionName, message, replacement);
        } else if (clazy::startsWith(className, "class QSignalMapper") && functionName == "mapped") {
            replacementForQSignalMapper(membExpr, message, replacement);
        } else if (clazy::startsWith(className, "QSet") && qSetDeprecatedFunctions.find(functionName) != qSetDeprecatedFunctions.end()) {
            message = "QSet iterator categories changed from bidirectional to forward. Please port your code manually";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QHash") && qHashDeprecatedFunctions.find(functionName) != qHashDeprecatedFunctions.end()) {
            message = "QHash iterator categories changed from bidirectional to forward. Please port your code manually";
            emitWarning(warningLocation, message, fixits);
            return;
        }else {
            return;
        }
        fixitRange = SourceRange(membExpr->getEndLoc());
    }else {
        return;
    }

    fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
    emitWarning(warningLocation, message, fixits);

    return;
}

void Qt6DeprecatedAPIFixes::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
