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

void replacementForQWizard(string functionName, string &message, string &replacement) {
    message = "call function QProcess::";
    message += functionName;
    message += "(). Use function QProcess::visitedIds() instead";

    replacement = "visitedIds";
}

bool replacementForQDate(clang::Stmt *parent, string &message, string &replacement, SourceLocation &warningLocation, SourceRange &fixitRange) {

    // The one with two arguments: Qt::DateFormat format, QCalendar cal
    CXXMemberCallExpr *callExp = dyn_cast<CXXMemberCallExpr>(parent);
    if (!callExp)
        return false;
    auto func = callExp->getDirectCallee();
    if (!func)
        return false;
    int i = 1;
    if (func->getNumParams() != 2)
        return false;
    for (auto it = func->param_begin() ; it !=func->param_end() ; it++) {
        ParmVarDecl *param = *it;
        if (i == 1 && param->getType().getAsString() != "Qt::DateFormat")
            return false;
        if (i == 2 && param->getType().getAsString() != "class QCalendar")
            return false;
        i++;
    }
    Stmt * firstArg = clazy::childAt(parent, 1);
    Stmt * secondArg = clazy::childAt(parent, 2);
    DeclRefExpr *declFirstArg = dyn_cast<DeclRefExpr>(firstArg);
    if (!firstArg || !secondArg || !declFirstArg)
        return false;
    fixitRange = SourceRange(firstArg->getEndLoc(), secondArg->getEndLoc());
    message = "replacing with function omitting the calendar. Change manually and use QLocale if you want to keep the calendar.";
    warningLocation = secondArg->getBeginLoc();
    replacement  = declFirstArg->getNameInfo().getAsString();
    return true;
}

static std::set<std::string> qButtonGroupDeprecatedFunctions = {"buttonClicked", "buttonPressed", "buttonReleased", "buttonToggled"};

bool replacementForQButtonGroup(clang::MemberExpr * membExpr, string &message, string &replacement) {

    auto declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    string paramType;
    for (auto param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString();
        break;
    }
    // only the function with "int" as first argument are deprecated
    if (paramType != "int")
        return false;

    string functionName = membExpr->getMemberNameInfo().getAsString();
    string newFunctionName = "id";
    newFunctionName += functionName.substr(6,8);

    message = "call function QButtonGroup::";
    message += functionName;
    message += "(int";
    if (declfunc->param_size() > 1)
        message += ", bool";
    message +="). Use function QButtonGroup";
    message += newFunctionName;
    message += " instead.";

    replacement = newFunctionName;
    return true;
}

bool warningForQTextBrowser(clang::MemberExpr * membExpr, string &message) {

    auto declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    string paramType;
    for (auto param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString();
        break;
    }
    if (paramType != "const class QString &")
        return false;

    message = "Using QTextBrowser::highlighted(const QString &). Use QTextBrowser::highlighted(const QUrl &) instead.";
    return true;
}

bool warningForQComboBox(clang::MemberExpr * membExpr, string &message) {

    auto declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    string paramType;
    for (auto param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString();
        break;
    }
    // only the function with "const QString &" as first argument are deprecated
    if (paramType != "const class QString &")
        return false;

    message = "Use currentIndexChanged(int) instead, and get the text using itemText(index).";
    return true;
}

bool replacementForQComboBox(clang::MemberExpr * membExpr, string functionName, string &message, string &replacement) {

    auto declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    string paramType;
    for (auto param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString();
        break;
    }
    // only the function with "const QString &" as first argument are deprecated
    if (paramType != "const class QString &")
        return false;

    if (functionName == "activated") {
        message = "Using QComboBox::activated(const QString &). Use textActiated() instead";
        replacement = "textActivated";
    } else if (functionName == "highlighted") {
        message = "Using QComboBox::hilighted(const QString &). Use textHighlighted() instead";
        replacement = "textHighlighted";
    } else {
        return false;
    }
    return true;
}

static std::set<std::string> qProcessDeprecatedFunctions = {"start"};

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
        functionNameExtention = "Object";
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

static std::set<std::string> qGraphicsViewFunctions = {"matrix", "setMatrix", "resetMatrix"};

void warningForGraphicsViews(string functionName, string &message) {

    if (functionName == "matrix") {
        message = "Using QGraphicsView::matrix. Use transform() instead";
        return;
    } else if (functionName == "setMatrix") {
        message = "Using QGraphicsView::setMatrix(const QMatrix &). Use setTransform(const QTransform &) instead";
        return;
    } else if (functionName == "resetMatrix") {
        message = "Using QGraphicsView::resetMatrix(). Use resetTransform() instead";
        return;
    }
    return;
}

static std::set<std::string> qStylePixelMetrix = {"PM_DefaultTopLevelMargin", "PM_DefaultChildMargin", "PM_DefaultLayoutSpacing"};

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

bool getMessageForDeclWarning(string type, string &message)
{
    if (clazy::contains(type, "QLinkedList")) {
        message = "Using QLinkedList. Use std::list instead";
        return true;
    } else if (clazy::contains(type, "QMacCocoaViewContainer")) {
        message =  "Using QMacCocoaViewContainer."
               " Use QWindow::fromWinId and QWidget::createWindowContainer instead";
        return true;
    } else if (clazy::contains(type, "QMacNativeWidget")) {
        message =  "Using QMacNativeWidget."
               " Use QWidget::winId instead";
        return true;
    } else if (clazy::contains(type, "QDirModel")) {
        message =  "Using QDirModel."
               " Use QFileSystemModel instead";
        return true;
    } else if (clazy::contains(type, "QString::SplitBehavior")) {
        message = "Using QString::SplitBehavior. Use Qt::SplitBehavior variant instead";
        return true;
    }else {
        return false;
    }

}

void Qt6DeprecatedAPIFixes::VisitDecl(clang::Decl *decl)
{
    auto funcDecl = decl->getAsFunction();
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    FieldDecl *fieldDecl = dyn_cast<FieldDecl>(decl);

    if (!funcDecl && !varDecl && !fieldDecl)
        return;

    DeclaratorDecl *declaratorDecl = nullptr;
    QualType qualType;
    if (funcDecl) {
        declaratorDecl = funcDecl;
        qualType = funcDecl->getReturnType();
    } else if (varDecl) {
        declaratorDecl = varDecl;
        qualType = varDecl->getType();
    } else if (fieldDecl) {
        declaratorDecl = fieldDecl;
        qualType = fieldDecl->getType();
    }

    string message;
    if (!getMessageForDeclWarning(qualType.getAsString(), message))
        return;

#if LLVM_VERSION_MAJOR >= 10
    const string type = qualType.getAsString();
    vector<FixItHint> fixits;

    if (clazy::endsWith(type, "QString::SplitBehavior")) {
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
        string replacement;
        if (!isQtNamespaceExplicit)
            replacement = "Qt::";
        replacement += "SplitBehavior";
        SourceRange sourceRange(declaratorDecl->getTypeSpecStartLoc(), declaratorDecl->getTypeSpecEndLoc());
        fixits.push_back(FixItHint::CreateReplacement(sourceRange, replacement));
    }
#endif

    emitWarning(decl->getBeginLoc(), message, fixits);
    return;
}

string Qt6DeprecatedAPIFixes::buildReplacementforQDir(Stmt* stmt, DeclRefExpr* declb)
{
    string replacement = declb->getNameInfo().getAsString();
    QualType qualtype = declb->getType();
    if (qualtype->isPointerType())
        replacement += "->";
    else
    replacement += ".";
    replacement += "setPath(";
    replacement += findPathArgument(clazy::childAt(stmt, 2));
    replacement += ")";
    return replacement;
}

string Qt6DeprecatedAPIFixes::buildReplacementForQVariant(Stmt* stmt, DeclRefExpr* decl,  DeclRefExpr* declb)
{
    string replacement = "QVariant::compare(";
    QualType qualtype = declb->getType();
    if (qualtype->isPointerType())
        replacement += "*";
    replacement += declb->getNameInfo().getAsString();
    replacement += ", ";
    replacement += findPathArgument(clazy::childAt(stmt, 2));
    replacement += ") ";
    replacement += decl->getNameInfo().getAsString().substr(8,2);
    replacement += " 0" ;
    return replacement;
}

bool foundQDirDeprecatedOperator(DeclRefExpr *decl)
{
    if (decl->getNameInfo().getAsString() == "operator=" )
         return true;
    else
        return false;
}

static std::set<std::string> qVariantDeprecatedOperator = {"operator<", "operator<=", "operator>", "operator>="};

bool foundQVariantDeprecatedOperator(DeclRefExpr *decl)
{
    if (qVariantDeprecatedOperator.find(decl->getNameInfo().getAsString()) != qTextStreamFunctions.end())
        return true;
    else
        return false;
}

void Qt6DeprecatedAPIFixes::fixForDeprecatedOperator(Stmt* stmt, string className)
{
    // only interested in '=' operator for QDir
    vector<FixItHint> fixits;
    string message;
    string replacement;
    SourceLocation warningLocation;
    SourceRange fixitRange;
    Stmt *child = clazy::childAt(stmt, 0);
    bool foundOperator = false;
    DeclRefExpr *decl = nullptr;
    while (child) {
        decl = dyn_cast<DeclRefExpr>(child);
        if ( !decl ) {
            child = clazy::childAt(child, 0);
            continue;
        }

        if (className == "QDir") {
                foundOperator = foundQDirDeprecatedOperator(decl);
        }
        else if (className == "QVariant") {
                foundOperator = foundQVariantDeprecatedOperator(decl);
        }

        if (foundOperator) {
            warningLocation = decl->getLocation();
            break;
        }
        child = clazy::childAt(child, 0);
    }

    if (!foundOperator)
        return;

    // get the name of the QDir variable from child2 value
    child = clazy::childAt(stmt, 1);
    DeclRefExpr *declb = nullptr;
    while (child) {
        declb = dyn_cast<DeclRefExpr>(child);
        if ( !declb ) {
            child = clazy::childAt(child, 0);
            continue;
        } else {
            break;
        }
    }
    if ( !declb )
        return;

    if (className == "QDir") {
        message = " function setPath() has to be used in Qt6";
        // need to make sure there is no macro
        for (auto macro_pos : m_listingMacroExpand) {
            if (m_sm.isPointWithin(macro_pos, clazy::getLocStart(stmt), clazy::getLocEnd(stmt))) {
               emitWarning(warningLocation, message, fixits);
               return;
            }
        }
            replacement = buildReplacementforQDir(stmt, declb);
     } else if (className == "QVariant") {
            message = " operator does not exist in Qt6. Using QVariant::compare() instead";
            replacement = buildReplacementForQVariant(stmt, decl, declb);
    }

    fixitRange = stmt->getSourceRange();
    fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
    emitWarning(warningLocation, message, fixits);

    return;

}

void Qt6DeprecatedAPIFixes::VisitStmt(clang::Stmt *stmt)
{
    CXXOperatorCallExpr *oppCallExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    DeclRefExpr *declRefExp = dyn_cast<DeclRefExpr>(stmt);
    MemberExpr *membExpr = dyn_cast<MemberExpr>(stmt);
    CXXConstructExpr *consExpr = dyn_cast<CXXConstructExpr>(stmt);

    SourceLocation warningLocation;
    std::string replacement;
    std::string message;
    SourceRange fixitRange;

    vector<FixItHint> fixits;

    if (consExpr) {
        auto constructor = consExpr->getConstructor();
        if (!constructor)
            return;
        if (constructor->getDeclName().getAsString() == "QSplashScreen") {
            if (consExpr->getNumArgs() == 0)
                return;
            if (consExpr->getArg(0)->getType().getAsString() != "class QWidget *")
                return;
            message = "Use the constructor taking a QScreen * instead.";
            warningLocation = stmt->getBeginLoc();
            emitWarning(warningLocation, message, fixits);
            return;
        }
        if (constructor->getDeclName().getAsString() != "QDateTime")
            return;
        if ( consExpr->getNumArgs() != 1)
            return;
        if ( consExpr->getArg(0)->getType().getAsString() != "const class QDate")
            return;
        Stmt *child = clazy::childAt(stmt, 0);
        DeclRefExpr *decl;
        while(child) {
            decl = dyn_cast<DeclRefExpr>(child);
            if ( !decl ) {
                child = clazy::childAt(child, 0);
                continue;
            } else {
                break;
            }
        }
        if (!decl)
            return;

        replacement = decl->getNameInfo().getAsString();
        QualType qualtype = decl->getType();
        if (qualtype->isPointerType())
            replacement += "->";
        else
        replacement += ".";
        replacement += "startOfDay()";

        warningLocation = stmt->getBeginLoc();
        fixitRange = stmt->getSourceRange();
        message = "deprecated constructor. Use QDate::startOfDay() instead.";
    } else if (oppCallExpr) {

        if ( clazy::isOfClass(oppCallExpr, "QDir")) {
            fixForDeprecatedOperator(stmt, "QDir");
            return;
        } else if (clazy::isOfClass(oppCallExpr, "QVariant")) {
            fixForDeprecatedOperator(stmt, "QVariant");
            return;
        }
        return;

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
        if (functionName == "qrand" || functionName == "qsrand") {
            // To catch qrand and qsrand from qglobal.
            message = "use QRandomGenerator instead";
            emitWarning(warningLocation, message, fixits);
            return;
        }

        string declType;
        declType = decl->getType().getAsString();
        if (functionName == "AdjustToMinimumContentsLength" && declType == "enum QComboBox::SizeAdjustPolicy") {
            message = "Use QComboBox::SizeAdjustPolicy::AdjustToContents or AdjustToContentsOnFirstShow instead";
            emitWarning(warningLocation, message, fixits);
            return;
        }

        if (functionName == "AllDockWidgetFeatures" && declType == "enum QDockWidget::DockWidgetFeature") {
            message = "Use QComboBox::DockWidgetClosable|DockWidgetMovable|DockWidgetFloatable explicitly instead.";
            emitWarning(warningLocation, message, fixits);
            return;
        }

        if ((qStylePixelMetrix.find(functionName) != qStylePixelMetrix.end() && declType == "enum QStyle::PixelMetric") ||
            (functionName == "SE_DialogButtonBoxLayoutItem" && declType == "enum QStyle::SubElement")) {
            message = "this enum has been removed in Qt6";
            emitWarning(warningLocation, message, fixits);
            return;
        }


        if (functionName != "MatchRegExp" && enclosingNameSpace != "QTextStreamFunctions" &&
            functionName != "KeepEmptyParts" && functionName != "SkipEmptyParts")
            return;

        // To catch enum Qt::MatchFlag and enum QString::SplitBehavior
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
        DeclRefExpr *decl = nullptr;
        while (child) {
            decl = dyn_cast<DeclRefExpr>(child);
            if (decl)
                break;
            child = clazy::childAt(child, 0);
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
        } else if (clazy::startsWith(className, "class QTimeLine") &&
                   (functionName == "curveShape" || functionName == "setCurveShape")) {
            if (functionName == "curveShape") {
                message = "call QTimeLine::curveShape. Use QTimeLine::easingCurve instead";
            } else {
                message = "call QTimeLine::setCurveShape. Use QTimeLine::setEasingCurve instead";
            }
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QSet") && qSetDeprecatedFunctions.find(functionName) != qSetDeprecatedFunctions.end()) {
            message = "QSet iterator categories changed from bidirectional to forward. Please port your code manually";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QHash") && qHashDeprecatedFunctions.find(functionName) != qHashDeprecatedFunctions.end()) {
            message = "QHash iterator categories changed from bidirectional to forward. Please port your code manually";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "class QComboBox") && functionName == "currentIndexChanged") {
            if (!warningForQComboBox(membExpr, message))
                return;
            emitWarning(warningLocation, message, fixits);
            return;
        }  else if (clazy::startsWith(className, "class QTextBrowser") && functionName == "highlighted") {
            if (!warningForQTextBrowser(membExpr, message))
                return;
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "class QGraphicsView") &&
                   qGraphicsViewFunctions.find(functionName) != qMapFunctions.end()) {
            warningForGraphicsViews(functionName, message);
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (className == "class QDate"  && functionName == "toString") {
            Stmt *parent = clazy::parent(m_context->parentMap, stmt);
            if (!replacementForQDate(parent, message, replacement, warningLocation, fixitRange))
                return;
            fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
            emitWarning(warningLocation, message, fixits);
            return;
        }else if (clazy::startsWith(className, "class QProcess") &&
                   qProcessDeprecatedFunctions.find(functionName) != qProcessDeprecatedFunctions.end()) {
            replacementForQProcess(functionName, message, replacement);
        } else if (clazy::startsWith(className, "class QResource") && functionName == "isCompressed") {
            replacementForQResource(functionName, message, replacement);
        } else if (clazy::startsWith(className, "class QSignalMapper") && functionName == "mapped") {
            replacementForQSignalMapper(membExpr, message, replacement);
        } else if (clazy::startsWith(className, "class QWizard") && functionName == "visitedPages") {
            replacementForQWizard(functionName, message, replacement);
        } else if (clazy::startsWith(className, "class QButtonGroup") &&
                   qButtonGroupDeprecatedFunctions.find(functionName) != qButtonGroupDeprecatedFunctions.end()) {
            if (!replacementForQButtonGroup(membExpr, message, replacement))
                return;
        } else if (clazy::startsWith(className, "class QComboBox") &&
                   (functionName == "activated" || functionName == "highlighted")) {
            if (!replacementForQComboBox(membExpr, functionName, message, replacement))
                return;
        } else {
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
