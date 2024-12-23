/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt6-deprecated-api-fixes.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

Qt6DeprecatedAPIFixes::Qt6DeprecatedAPIFixes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

void replacementForQWizard(const std::string &functionName, std::string &message, std::string &replacement)
{
    message = "call function QProcess::";
    message += functionName;
    message += "(). Use function QProcess::visitedIds() instead";

    replacement = "visitedIds";
}

bool replacementForQDate(clang::Stmt *parent,
                         std::string &message,
                         std::string &replacement,
                         SourceLocation &warningLocation,
                         SourceRange &fixitRange,
                         LangOptions lo)
{
    // The one with two arguments: Qt::DateFormat format, QCalendar cal
    auto *callExp = dyn_cast<CXXMemberCallExpr>(parent);
    if (!callExp) {
        return false;
    }
    auto *func = callExp->getDirectCallee();
    if (!func) {
        return false;
    }
    int i = 1;
    if (func->getNumParams() != 2) {
        return false;
    }
    for (auto *it = func->param_begin(); it != func->param_end(); it++) {
        ParmVarDecl *param = *it;
        if (i == 1 && param->getType().getAsString(lo) != "Qt::DateFormat") {
            return false;
        }
        if (i == 2 && param->getType().getAsString(lo) != "QCalendar") {
            return false;
        }
        i++;
    }
    Stmt *firstArg = clazy::childAt(parent, 1);
    Stmt *secondArg = clazy::childAt(parent, 2);
    auto *declFirstArg = dyn_cast<DeclRefExpr>(firstArg);
    if (!firstArg || !secondArg || !declFirstArg) {
        return false;
    }
    fixitRange = SourceRange(firstArg->getEndLoc(), secondArg->getEndLoc());
    message = "replacing with function omitting the calendar. Change manually and use QLocale if you want to keep the calendar.";
    warningLocation = secondArg->getBeginLoc();
    replacement = declFirstArg->getNameInfo().getAsString();
    return true;
}

static std::set<std::string> qButtonGroupDeprecatedFunctions = {"buttonClicked", "buttonPressed", "buttonReleased", "buttonToggled"};

bool replacementForQButtonGroup(clang::MemberExpr *membExpr, std::string &message, std::string &replacement)
{
    auto *declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    std::string paramType;
    for (auto *param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString();
        break;
    }
    // only the function with "int" as first argument are deprecated
    if (paramType != "int") {
        return false;
    }

    std::string functionName = membExpr->getMemberNameInfo().getAsString();
    std::string newFunctionName = "id";
    newFunctionName += functionName.substr(6, 8);

    message = "call function QButtonGroup::";
    message += functionName;
    message += "(int";
    if (declfunc->param_size() > 1) {
        message += ", bool";
    }
    message += "). Use function QButtonGroup";
    message += newFunctionName;
    message += " instead.";

    replacement = newFunctionName;
    return true;
}

inline bool isFirstArgQStringConstRef(FunctionDecl *declfunc, LangOptions lo)
{
    auto params = Utils::functionParameters(declfunc);
    return !params.empty() && params.front()->getType().getAsString(lo) == "const QString &";
}

bool warningForQTextBrowser(clang::MemberExpr *membExpr, std::string &message, LangOptions lo)
{
    auto *declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    if (isFirstArgQStringConstRef(declfunc, lo)) {
        message = "Using QTextBrowser::highlighted(const QString &). Use QTextBrowser::highlighted(const QUrl &) instead.";
        return true;
    }
    return false;
}

bool warningForQComboBox(clang::MemberExpr *membExpr, std::string &message, LangOptions lo)
{
    auto *declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    if (isFirstArgQStringConstRef(declfunc, lo)) {
        message = "Use currentIndexChanged(int) instead, and get the text using itemText(index).";
        return true;
    }
    return false;
}

bool replacementForQComboBox(clang::MemberExpr *membExpr, const std::string &functionName, std::string &message, std::string &replacement, LangOptions lo)
{
    auto *declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    if (!isFirstArgQStringConstRef(declfunc, lo)) {
        return false;
    }

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

void replacementForQProcess(const std::string &functionName, std::string &message, std::string &replacement)
{
    message = "call function QProcess::";
    message += functionName;
    message += "(). Use function QProcess::";
    message += functionName;
    message += "Command() instead";

    replacement = functionName;
    replacement += "Command";
}

void replacementForQSignalMapper(clang::MemberExpr *membExpr, std::string &message, std::string &replacement, clang::LangOptions lo)
{
    auto *declfunc = membExpr->getReferencedDeclOfCallee()->getAsFunction();
    std::string paramType;
    for (auto *param : Utils::functionParameters(declfunc)) {
        paramType = param->getType().getAsString(lo);
    }

    std::string functionNameExtention;
    if (paramType == "int") {
        functionNameExtention = "Int";
    } else if (paramType == "const QString &") {
        functionNameExtention = "String";
    } else if (paramType == "QWidget *") {
        functionNameExtention = "Object";
    } else if (paramType == "QObject *") {
        functionNameExtention = "Object";
    }

    message = "call function QSignalMapper::mapped(";
    message += paramType;
    message += "). Use function QSignalMapper::mapped";
    message += functionNameExtention;
    message += "(";
    message += paramType;
    message += ") instead.";

    replacement = "mapped";
    replacement += functionNameExtention;
}

void replacementForQResource(const std::string & /*functionName*/, std::string &message, std::string &replacement)
{
    message = "call function QRessource::isCompressed(). Use function QProcess::compressionAlgorithm() instead.";
    replacement = "compressionAlgorithm";
}

static std::set<std::string> qSetDeprecatedOperators = {"operator--", "operator+", "operator-", "operator+=", "operator-="};
static std::set<std::string> qSetDeprecatedFunctions = {"rbegin", "rend", "crbegin", "crend", "hasPrevious", "previous", "peekPrevious", "findPrevious"};
static std::set<std::string> qHashDeprecatedFunctions = {"hasPrevious", "previous", "peekPrevious", "findPrevious"};

bool isQSetDepreprecatedOperator(const std::string &functionName, const std::string &contextName, std::string &message)
{
    if (qSetDeprecatedOperators.find(functionName) == qSetDeprecatedOperators.end()) {
        return false;
    }
    if ((clazy::startsWith(contextName, "QSet<") || clazy::startsWith(contextName, "QHash<")) && clazy::endsWith(contextName, "iterator")) {
        if (clazy::startsWith(contextName, "QSet<")) {
            message = "QSet iterator categories changed from bidirectional to forward. Please port your code manually";
        } else {
            message = "QHash iterator categories changed from bidirectional to forward. Please port your code manually";
        }

        return true;
    }
    return false;
}

static std::set<std::string> qGraphicsViewFunctions = {"matrix", "setMatrix", "resetMatrix"};

void warningForGraphicsViews(const std::string &functionName, std::string &message)
{
    if (functionName == "matrix") {
        message = "Using QGraphicsView::matrix. Use transform() instead";
        return;
    }
    if (functionName == "setMatrix") {
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

static std::set<std::string> qTextStreamFunctions = {"bin",           "oct",
                                                     "dec",           "hex",
                                                     "showbase",      "forcesign",
                                                     "forcepoint",    "noshowbase",
                                                     "noforcesign",   "noforcepoint",
                                                     "uppercasebase", "uppercasedigits",
                                                     "lowercasebase", "lowercasedigits",
                                                     "fixed",         "scientific",
                                                     "left",          "right",
                                                     "center",        "endl",
                                                     "flush",         "reset",
                                                     "bom",           "ws"};

void replacementForQTextStreamFunctions(const std::string &functionName, std::string &message, std::string &replacement, bool explicitQtNamespace)
{
    if (qTextStreamFunctions.find(functionName) == qTextStreamFunctions.end()) {
        return;
    }
    message = "call function QTextStream::";
    message += functionName;
    message += ". Use function Qt::";
    message += functionName;
    message += " instead";

    if (!explicitQtNamespace) {
        replacement = "Qt::";
    }
    replacement += functionName;
}

void replacementForQStringSplitBehavior(const std::string &functionName, std::string &message, std::string &replacement, bool explicitQtNamespace)
{
    message = "Use Qt::SplitBehavior variant instead";
    if (!explicitQtNamespace) {
        replacement = "Qt::";
    }
    replacement += functionName;
}

bool getMessageForDeclWarning(const std::string &type, std::string &message)
{
    if (clazy::contains(type, "QLinkedList")) {
        message = "Using QLinkedList. Use std::list instead";
        return true;
    }
    if (clazy::contains(type, "QMacCocoaViewContainer")) {
        message =
            "Using QMacCocoaViewContainer."
            " Use QWindow::fromWinId and QWidget::createWindowContainer instead";
        return true;
    } else if (clazy::contains(type, "QMacNativeWidget")) {
        message =
            "Using QMacNativeWidget."
            " Use QWidget::winId instead";
        return true;
    } else if (clazy::contains(type, "QDirModel")) {
        message =
            "Using QDirModel."
            " Use QFileSystemModel instead";
        return true;
    } else if (clazy::contains(type, "QString::SplitBehavior")) {
        message = "Using QString::SplitBehavior. Use Qt::SplitBehavior variant instead";
        return true;
    } else {
        return false;
    }
}

void Qt6DeprecatedAPIFixes::VisitDecl(clang::Decl *decl)
{
    auto *funcDecl = decl->getAsFunction();
    auto *varDecl = dyn_cast<VarDecl>(decl);
    auto *fieldDecl = dyn_cast<FieldDecl>(decl);

    if (!funcDecl && !varDecl && !fieldDecl) {
        return;
    }

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

    std::string message;
    if (!getMessageForDeclWarning(qualType.getAsString(), message)) {
        return;
    }

    std::vector<FixItHint> fixits;

    const std::string type = qualType.getAsString();
    if (clazy::endsWith(type, "QString::SplitBehavior")) {
        bool isQtNamespaceExplicit = false;
        DeclContext *newcontext = clazy::contextForDecl(m_context->lastDecl);
        while (newcontext) {
            if (!newcontext) {
                break;
            }
            if (clang::isa<NamespaceDecl>(newcontext)) {
                auto *namesdecl = dyn_cast<clang::NamespaceDecl>(newcontext);
                if (namesdecl->getNameAsString() == "Qt") {
                    isQtNamespaceExplicit = true;
                }
            }
            newcontext = newcontext->getParent();
        }
        std::string replacement;
        if (!isQtNamespaceExplicit) {
            replacement = "Qt::";
        }
        replacement += "SplitBehavior";
        SourceRange sourceRange(declaratorDecl->getTypeSpecStartLoc(), declaratorDecl->getTypeSpecEndLoc());
        fixits.push_back(FixItHint::CreateReplacement(sourceRange, replacement));
    }

    emitWarning(decl->getBeginLoc(), message, fixits);
    return;
}

std::string
Qt6DeprecatedAPIFixes::buildReplacementforQDir(DeclRefExpr * /*decl_operator*/, bool isPointer, std::string replacement, const std::string &replacement_var2)
{
    if (isPointer) {
        replacement += "->";
    } else {
        replacement += ".";
    }
    replacement += "setPath(";
    replacement += replacement_var2;
    replacement += ")";
    return replacement;
}

std::string
Qt6DeprecatedAPIFixes::buildReplacementForQVariant(DeclRefExpr *decl_operator, const std::string &replacement_var1, const std::string &replacement_var2)
{
    std::string replacement = "QVariant::compare(";
    replacement += replacement_var1;
    replacement += ", ";
    replacement += replacement_var2;
    replacement += ") ";
    replacement += decl_operator->getNameInfo().getAsString().substr(8, 2);
    replacement += " 0";
    return replacement;
}

bool foundQDirDeprecatedOperator(DeclRefExpr *decl)
{
    return decl->getNameInfo().getAsString() == "operator=";
}

static std::set<std::string> qVariantDeprecatedOperator = {"operator<", "operator<=", "operator>", "operator>="};

bool foundQVariantDeprecatedOperator(DeclRefExpr *decl)
{
    return qVariantDeprecatedOperator.find(decl->getNameInfo().getAsString()) != qVariantDeprecatedOperator.end();
}

void Qt6DeprecatedAPIFixes::fixForDeprecatedOperator(Stmt *stmt, const std::string &className)
{
    // only interested in '=' operator for QDir
    std::vector<FixItHint> fixits;
    std::string message;
    std::string replacement;
    SourceLocation warningLocation;
    SourceRange fixitRange;
    Stmt *child = clazy::childAt(stmt, 0);
    bool foundOperator = false;
    DeclRefExpr *decl = nullptr;
    while (child) {
        decl = dyn_cast<DeclRefExpr>(child);
        if (!decl) {
            child = clazy::childAt(child, 0);
            continue;
        }

        if (className == "QDir") {
            foundOperator = foundQDirDeprecatedOperator(decl);
        } else if (className == "QVariant") {
            foundOperator = foundQVariantDeprecatedOperator(decl);
        }

        if (foundOperator) {
            warningLocation = decl->getLocation();
            break;
        }
        child = clazy::childAt(child, 0);
    }

    if (!foundOperator) {
        return;
    }

    // Getting the two arguments of the operator to build the replacement
    auto *oppCallExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    auto *arg0Size = oppCallExpr->getArg(0);
    auto *arg1Size = oppCallExpr->getArg(1);
    auto charRange = Lexer::getAsCharRange(arg0Size->getSourceRange(), m_sm, lo());
    auto replacementVar1 = Lexer::getSourceText(charRange, m_sm, lo());
    charRange = Lexer::getAsCharRange(arg1Size->getSourceRange(), m_sm, lo());
    auto replacementVar2 = Lexer::getSourceText(charRange, m_sm, lo());

    replacementVar1 = replacementVar1.rtrim(' ');
    replacementVar2 = replacementVar2.ltrim(' ');

    if (className == "QDir") {
        message = " function setPath() has to be used in Qt6";
        // Get the quality type of the operator first argument.
        // qdir_var1 = var2 => qdir_var1->setPath(var2) or qdir_var1.setPath(var2)
        // the qdir_var1 correspond to second child of the QDir operator
        child = clazy::childAt(stmt, 1);
        bool isPointer = false;
        while (child) {
            auto *castExpr = dyn_cast<ImplicitCastExpr>(child);
            auto *parent = dyn_cast<ParenExpr>(child);
            if (castExpr || parent) {
                child = clazy::childAt(child, 0);
                continue;
            }
            auto *uni = dyn_cast<UnaryOperator>(child);
            if (uni) {
#if LLVM_VERSION_MAJOR >= 19
#define STRING_EQUALS(a, b) a == b
#else
#define STRING_EQUALS(a, b) a.equals(b)
#endif
                if (STRING_EQUALS(clang::UnaryOperator::getOpcodeStr(uni->getOpcode()), "*")) {
                    isPointer = true;
                }
            }
            break;
        }
        if (isPointer) {
            while (replacementVar1.consume_front("(")) {
                replacementVar1.consume_back(")");
            }
            replacementVar1.consume_front("*");
        }
        replacement = buildReplacementforQDir(decl, isPointer, replacementVar1.str(), replacementVar2.str());
    } else if (className == "QVariant") {
        message = " operator does not exist in Qt6. Using QVariant::compare() instead.";
        replacement = buildReplacementForQVariant(decl, replacementVar1.str(), replacementVar2.str());
    }

    // If a macro is present in the stmt range the spelling location is used
    // This is producing a wrong fix. So we're forcing the use of expansion location
    FullSourceLoc endLoc(stmt->getEndLoc(), m_sm);
    SourceRange range(stmt->getBeginLoc(), endLoc.getExpansionLoc());
    fixitRange = range;
    fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
    emitWarning(warningLocation, message, fixits);

    return;
}

void Qt6DeprecatedAPIFixes::VisitStmt(clang::Stmt *stmt)
{
    auto *oppCallExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    auto *declRefExp = dyn_cast<DeclRefExpr>(stmt);
    auto *membExpr = dyn_cast<MemberExpr>(stmt);
    auto *consExpr = dyn_cast<CXXConstructExpr>(stmt);

    SourceLocation warningLocation;
    std::string replacement;
    std::string message;
    SourceRange fixitRange;

    std::vector<FixItHint> fixits;

    if (consExpr) {
        auto *constructor = consExpr->getConstructor();
        if (!constructor) {
            return;
        }
        if (constructor->getDeclName().getAsString() == "QSplashScreen") {
            if (consExpr->getNumArgs() == 0) {
                return;
            }
            if (consExpr->getArg(0)->getType().getAsString(lo()) != "QWidget *") {
                return;
            }
            message = "Use the constructor taking a QScreen * instead.";
            warningLocation = stmt->getBeginLoc();
            emitWarning(warningLocation, message, fixits);
            return;
        }
        if (constructor->getDeclName().getAsString() != "QDateTime") {
            return;
        }
        if (consExpr->getNumArgs() != 1) {
            return;
        }
        if (consExpr->getArg(0)->getType().getAsString(lo()) != "const QDate") {
            return;
        }
        Stmt *child = clazy::childAt(stmt, 0);
        DeclRefExpr *decl;
        while (child) {
            decl = dyn_cast<DeclRefExpr>(child);
            if (!decl) {
                child = clazy::childAt(child, 0);
                continue;
            }
            break;
        }
        if (!decl) {
            return;
        }

        replacement = decl->getNameInfo().getAsString();
        QualType qualtype = decl->getType();
        if (qualtype->isPointerType()) {
            replacement += "->";
        } else {
            replacement += ".";
        }
        replacement += "startOfDay()";

        warningLocation = stmt->getBeginLoc();
        fixitRange = stmt->getSourceRange();
        message = "deprecated constructor. Use QDate::startOfDay() instead.";
    } else if (oppCallExpr) {
        if (clazy::isOfClass(oppCallExpr, "QDir")) {
            fixForDeprecatedOperator(stmt, "QDir");
            return;
        }
        if (clazy::isOfClass(oppCallExpr, "QVariant")) {
            fixForDeprecatedOperator(stmt, "QVariant");
            return;
        }
        return;

    } else if (declRefExp) {
        warningLocation = declRefExp->getBeginLoc();
        auto *decl = declRefExp->getDecl();
        if (!decl) {
            return;
        }
        auto *declContext = declRefExp->getDecl()->getDeclContext();
        if (!declContext) {
            return;
        }
        std::string functionName = declRefExp->getNameInfo().getAsString();
        std::string enclosingNameSpace;
        auto *enclNsContext = declContext->getEnclosingNamespaceContext();
        if (enclNsContext) {
            if (auto *ns = dyn_cast<NamespaceDecl>(declContext->getEnclosingNamespaceContext())) {
                enclosingNameSpace = ns->getNameAsString();
            }
        }

        // To catch QDir and QSet
        std::string contextName;
        if (declContext) {
            if (clang::isa<clang::CXXRecordDecl>(declContext)) {
                auto *recordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(declContext);
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

        std::string declType;
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

        if ((qStylePixelMetrix.find(functionName) != qStylePixelMetrix.end() && declType == "enum QStyle::PixelMetric")
            || (functionName == "SE_DialogButtonBoxLayoutItem" && declType == "enum QStyle::SubElement")) {
            message = "this enum has been removed in Qt6";
            emitWarning(warningLocation, message, fixits);
            return;
        }

        if (functionName != "MatchRegExp" && enclosingNameSpace != "QTextStreamFunctions" && functionName != "KeepEmptyParts"
            && functionName != "SkipEmptyParts") {
            return;
        }

        // To catch enum Qt::MatchFlag and enum QString::SplitBehavior
        // Get out of this DeclRefExp to catch the potential Qt namespace surrounding it
        bool isQtNamespaceExplicit = false;
        DeclContext *newcontext = clazy::contextForDecl(m_context->lastDecl);
        while (newcontext) {
            if (!newcontext) {
                break;
            }
            if (clang::isa<NamespaceDecl>(newcontext)) {
                auto *namesdecl = dyn_cast<clang::NamespaceDecl>(newcontext);
                if (namesdecl->getNameAsString() == "Qt") {
                    isQtNamespaceExplicit = true;
                }
            }
            newcontext = newcontext->getParent();
        }

        if (enclosingNameSpace == "QTextStreamFunctions") {
            replacementForQTextStreamFunctions(functionName, message, replacement, isQtNamespaceExplicit);
        } else if ((functionName == "KeepEmptyParts" || functionName == "SkipEmptyParts") && declType == "enum QString::SplitBehavior") {
            replacementForQStringSplitBehavior(functionName, message, replacement, isQtNamespaceExplicit);
        } else if (functionName == "MatchRegExp" && declType == "enum Qt::MatchFlag") {
            message = "call Qt::MatchRegExp. Use Qt::MatchRegularExpression instead.";
            if (isQtNamespaceExplicit) {
                replacement = "MatchRegularExpression";
            } else {
                replacement = "Qt::MatchRegularExpression";
            }
        } else {
            return;
        }
        fixitRange = declRefExp->getSourceRange();

    } else if (membExpr) {
        Stmt *child = clazy::childAt(stmt, 0);
        DeclRefExpr *decl = nullptr;
        while (child) {
            decl = dyn_cast<DeclRefExpr>(child);
            if (decl) {
                break;
            }
            child = clazy::childAt(child, 0);
        }

        if (!decl) {
            return;
        }
        std::string functionName = membExpr->getMemberNameInfo().getAsString();
        std::string className = decl->getType().getAsString(lo());
        warningLocation = membExpr->getEndLoc();

        if (clazy::startsWith(className, "QMap<") && qMapFunctions.find(functionName) != qMapFunctions.end()) {
            message = "Use QMultiMap for maps storing multiple values with the same key.";
            emitWarning(warningLocation, message, fixits);
            return;
        }
        if (clazy::startsWith(className, "QHash<") && qMapFunctions.find(functionName) != qMapFunctions.end()) {
            // the name of the deprecated function are the same
            message = "Use QMultiHash for maps storing multiple values with the same key.";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QDir") && functionName == "addResourceSearchPath") {
            message = "call function QDir::addResourceSearchPath(). Use function QDir::addSearchPath() with prefix instead";
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QTimeLine") && (functionName == "curveShape" || functionName == "setCurveShape")) {
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
        } else if (clazy::startsWith(className, "QComboBox") && functionName == "currentIndexChanged") {
            if (!warningForQComboBox(membExpr, message, lo())) {
                return;
            }
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QTextBrowser") && functionName == "highlighted") {
            if (!warningForQTextBrowser(membExpr, message, lo())) {
                return;
            }
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QGraphicsView") && qGraphicsViewFunctions.find(functionName) != qGraphicsViewFunctions.end()) {
            warningForGraphicsViews(functionName, message);
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (className == "QDate" && functionName == "toString") {
            Stmt *parent = clazy::parent(m_context->parentMap, stmt);
            if (!replacementForQDate(parent, message, replacement, warningLocation, fixitRange, lo())) {
                return;
            }
            fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
            emitWarning(warningLocation, message, fixits);
            return;
        } else if (clazy::startsWith(className, "QProcess") && qProcessDeprecatedFunctions.find(functionName) != qProcessDeprecatedFunctions.end()) {
            replacementForQProcess(functionName, message, replacement);
        } else if (clazy::startsWith(className, "QResource") && functionName == "isCompressed") {
            replacementForQResource(functionName, message, replacement);
        } else if (clazy::startsWith(className, "QSignalMapper") && functionName == "mapped") {
            replacementForQSignalMapper(membExpr, message, replacement, lo());
        } else if (clazy::startsWith(className, "QWizard") && functionName == "visitedPages") {
            replacementForQWizard(functionName, message, replacement);
        } else if (clazy::startsWith(className, "QButtonGroup")
                   && qButtonGroupDeprecatedFunctions.find(functionName) != qButtonGroupDeprecatedFunctions.end()) {
            if (!replacementForQButtonGroup(membExpr, message, replacement)) {
                return;
            }
        } else if (clazy::startsWith(className, "QComboBox") && (functionName == "activated" || functionName == "highlighted")) {
            if (!replacementForQComboBox(membExpr, functionName, message, replacement, lo())) {
                return;
            }
        } else {
            return;
        }
        fixitRange = SourceRange(membExpr->getEndLoc());
    } else {
        return;
    }

    fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
    emitWarning(warningLocation, message, fixits);

    return;
}

void Qt6DeprecatedAPIFixes::VisitMacroExpands(const clang::Token & /*MacroNameTok*/, const clang::SourceRange &range, const MacroInfo *)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
