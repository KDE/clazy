/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "old-style-connect.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <regex>

using namespace clang;

namespace clazy
{
// Copied from Clang's Expr.cpp and added "|| !DerivedType->isRecordType()" to avoid a crash
const CXXRecordDecl *getBestDynamicClassType(Expr *expr)
{
    if (!expr) {
        return nullptr;
    }

    const Expr *E = expr->getBestDynamicClassTypeExpr();
    QualType DerivedType = E->getType();
    if (const auto *PTy = DerivedType->getAs<PointerType>()) {
        DerivedType = PTy->getPointeeType();
    }

    if (DerivedType->isDependentType() || !DerivedType->isRecordType()) {
        return nullptr;
    }

    const RecordType *Ty = DerivedType->castAs<RecordType>();
    Decl *D = Ty->getDecl();
    return cast<CXXRecordDecl>(D);
}
}

enum ConnectFlag {
    ConnectFlag_None = 0, // Not a disconnect or connect
    ConnectFlag_Connect = 1, // It's a connect
    ConnectFlag_Disconnect = 2, // It's a disconnect
    ConnectFlag_QTimerSingleShot = 4,
    ConnectFlag_OldStyle = 8, // Qt4 style
    ConnectFlag_4ArgsDisconnect = 16, // disconnect(const char *signal = 0, const QObject *receiver = 0, const char *method = 0) const
    ConnectFlag_3ArgsDisconnect = 32, // disconnect(SIGNAL(foo))
    ConnectFlag_2ArgsDisconnect = 64, // disconnect(const QObject *receiver, const char *method = 0) const
    ConnectFlag_5ArgsConnect =
        128, // connect(const QObject *sender, const char *signal, const QObject *receiver, const char *method, Qt::ConnectionType type = Qt::AutoConnection)
    ConnectFlag_4ArgsConnect = 256, // connect(const QObject *sender, const char *signal, const char *method, Qt::ConnectionType type = Qt::AutoConnection)
    ConnectFlag_OldStyleButNonLiteral = 0x200, // connect(foo, SIGNAL(bar()), foo, variableWithSlotName); // here the slot name isn't a literal
    ConnectFlag_QStateAddTransition = 0x400,
    ConnectFlag_QMenuAddAction = 0x800,
    ConnectFlag_QMessageBoxOpen = 0x1000,
    ConnectFlag_QSignalSpy = 0x2000,
    ConnectFlag_Bogus = 0x4000,
};

static bool classIsOk(StringRef className)
{
    // List of classes we usually use Qt4 syntax
    return className != "QDBusInterface";
}

OldStyleConnect::OldStyleConnect(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

template<typename T>
int OldStyleConnect::classifyConnect(FunctionDecl *connectFunc, T *connectCall) const
{
    int classification = ConnectFlag_None;

    const std::string methodName = connectFunc->getQualifiedNameAsString();
    if (methodName == "QObject::connect") {
        classification |= ConnectFlag_Connect;
    } else if (methodName == "QObject::disconnect") {
        classification |= ConnectFlag_Disconnect;
    } else if (methodName == "QTimer::singleShot") {
        classification |= ConnectFlag_QTimerSingleShot;
    } else if (methodName == "QState::addTransition") {
        classification |= ConnectFlag_QStateAddTransition;
    } else if (methodName == "QMenu::addAction" || methodName == "QWidget::addAction") {
        classification |= ConnectFlag_QMenuAddAction;
    } else if (methodName == "QMessageBox::open") {
        classification |= ConnectFlag_QMessageBoxOpen;
    } else if (methodName == "QSignalSpy::QSignalSpy") {
        classification |= ConnectFlag_QSignalSpy;
    }

    if (classification == ConnectFlag_None) {
        return classification;
    }

    if (clazy::connectHasPMFStyle(connectFunc)) {
        return classification;
    }
    classification |= ConnectFlag_OldStyle;

    const unsigned int numParams = connectFunc->getNumParams();

    if (classification & ConnectFlag_Connect) {
        if (numParams == 5) {
            classification |= ConnectFlag_5ArgsConnect;
        } else if (numParams == 4) {
            classification |= ConnectFlag_4ArgsConnect;
        } else {
            classification |= ConnectFlag_Bogus;
        }
    } else if (classification & ConnectFlag_Disconnect) {
        if (numParams == 4) {
            classification |= ConnectFlag_4ArgsDisconnect;
        } else if (numParams == 3) {
            classification |= ConnectFlag_3ArgsDisconnect;
        } else if (numParams == 2) {
            classification |= ConnectFlag_2ArgsDisconnect;
        } else {
            classification |= ConnectFlag_Bogus;
        }
    }

    if (classification & ConnectFlag_OldStyle) {
        // It's old style, but check if all macros are literals
        int numLiterals = 0;
        for (auto arg : connectCall->arguments()) {
            auto argLocation = arg->getBeginLoc();
            std::string dummy;
            if (isSignalOrSlot(argLocation, dummy)) {
                ++numLiterals;
            }
        }

        if ((classification & ConnectFlag_QTimerSingleShot) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if (((classification & ConnectFlag_Connect) && numLiterals != 2)) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_4ArgsDisconnect) && numLiterals != 2) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_QStateAddTransition) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_Disconnect) && numLiterals == 0) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_QMenuAddAction) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_QMessageBoxOpen) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_QSignalSpy) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        }
    }

    return classification;
}

bool OldStyleConnect::isQPointer(Expr *expr) const
{
    std::vector<CXXMemberCallExpr *> memberCalls;
    clazy::getChilds<CXXMemberCallExpr>(expr, memberCalls);

    return std::any_of(memberCalls.begin(), memberCalls.end(), [](CXXMemberCallExpr *callExpr) {
        auto *callee = callExpr->getDirectCallee();
        return callee && dyn_cast<CXXConversionDecl>(callee);
    });
}

bool OldStyleConnect::isPrivateSlot(const std::string &name) const
{
    return clazy::any_of(m_privateSlots, [name](const PrivateSlot &slot) {
        return slot.name == name;
    });
}

void OldStyleConnect::VisitStmt(Stmt *s)
{
    auto *call = dyn_cast<CallExpr>(s);
    auto *ctorExpr = call ? nullptr : dyn_cast<CXXConstructExpr>(s);
    if (!call && !ctorExpr) {
        return;
    }

    if (m_context->lastMethodDecl && m_context->isQtDeveloper() && m_context->lastMethodDecl->getParent()
        && clazy::name(m_context->lastMethodDecl->getParent()) == "QObject") { // Don't warn of stuff inside qobject.h
        return;
    }

    FunctionDecl *function = call ? call->getDirectCallee() : ctorExpr->getConstructor();
    if (!function) {
        return;
    }

    auto *method = dyn_cast<CXXMethodDecl>(function);
    if (!method) {
        return;
    }

    const int classification = call ? classifyConnect(method, call) : classifyConnect(method, ctorExpr);

    if (!(classification & ConnectFlag_OldStyle)) {
        return;
    }

    if ((classification & ConnectFlag_OldStyleButNonLiteral)) {
        return;
    }

    if (classification & ConnectFlag_Bogus) {
        emitWarning(s->getBeginLoc(), "Internal error");
        return;
    }

    emitWarning(s->getBeginLoc(), "Old Style Connect", call ? fixits(classification, call) : fixits(classification, ctorExpr));
}

void OldStyleConnect::addPrivateSlot(const PrivateSlot &slot)
{
    m_privateSlots.push_back(slot);
}

void OldStyleConnect::VisitMacroExpands(const Token &macroNameTok, const SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = macroNameTok.getIdentifierInfo();
    if (!ii || ii->getName() != "Q_PRIVATE_SLOT") {
        return;
    }

    auto charRange = Lexer::getAsCharRange(range, sm(), lo());
    const std::string text = static_cast<std::string>(Lexer::getSourceText(charRange, sm(), lo()));

    static std::regex rx(R"(Q_PRIVATE_SLOT\s*\((.*),.*\s(.*)\(.*)");
    std::smatch match;
    if (!regex_match(text, match, rx) || match.size() != 3) {
        return;
    }

    addPrivateSlot({match[1], match[2]});
}

// SIGNAL(foo()) -> foo
std::string OldStyleConnect::signalOrSlotNameFromMacro(SourceLocation macroLoc)
{
    if (!macroLoc.isMacroID()) {
        return "error";
    }

    CharSourceRange expansionRange = sm().getImmediateExpansionRange(macroLoc);
    SourceRange range = SourceRange(expansionRange.getBegin(), expansionRange.getEnd());
    auto charRange = Lexer::getAsCharRange(range, sm(), lo());
    const std::string text = static_cast<std::string>(Lexer::getSourceText(charRange, sm(), lo()));

    static std::regex rx(R"(\s*(SIGNAL|SLOT)\s*\(\s*(.+)\s*\(.*)");

    std::smatch match;
    if (regex_match(text, match, rx)) {
        if (match.size() == 3) {
            return match[2].str();
        }
        return "error2";

    } else {
        return std::string("regexp failed for ") + text;
    }
}

bool OldStyleConnect::isSignalOrSlot(SourceLocation loc, std::string &macroName) const
{
    macroName.clear();
    if (!loc.isMacroID() || loc.isInvalid()) {
        return false;
    }

    macroName = static_cast<std::string>(Lexer::getImmediateMacroName(loc, sm(), lo()));
    return macroName == "SIGNAL" || macroName == "SLOT";
}

template<typename T>
std::vector<FixItHint> OldStyleConnect::fixits(int classification, T *callOrCtor)
{
    if (!callOrCtor) {
        llvm::errs() << "Call is invalid\n";
        return {};
    }

    const SourceLocation locStart = callOrCtor->getBeginLoc();

    if (classification & ConnectFlag_2ArgsDisconnect) {
        // Not implemented yet
        std::string msg = "Fix it not implemented for disconnect with 2 args";
        queueManualFixitWarning(locStart, msg);
        return {};
    }

    if (classification & ConnectFlag_3ArgsDisconnect) {
        // Not implemented yet
        std::string msg = "Fix it not implemented for disconnect with 3 args";
        queueManualFixitWarning(locStart, msg);
        return {};
    }

    if (classification & ConnectFlag_QMessageBoxOpen) {
        std::string msg = "Fix it not implemented for QMessageBox::open()";
        queueManualFixitWarning(locStart, msg);
        return {};
    }

    std::vector<FixItHint> fixits;
    int macroNum = 0;
    std::string implicitCallee;
    std::string macroName;
    CXXMethodDecl *senderMethod = nullptr;
    for (auto arg : callOrCtor->arguments()) {
        SourceLocation s = arg->getBeginLoc();
        static const CXXRecordDecl *lastRecordDecl = nullptr;
        if (isSignalOrSlot(s, macroName)) {
            macroNum++;
            if (!lastRecordDecl && (classification & ConnectFlag_4ArgsConnect)) {
                // This means it's a connect with implicit receiver
                lastRecordDecl = Utils::recordForMemberCall(dyn_cast<CXXMemberCallExpr>(callOrCtor), implicitCallee);

                if (macroNum == 1) {
                    llvm::errs() << "This first macro shouldn't enter this path";
                }
                if (!lastRecordDecl) {
                    std::string msg = "Failed to get class name for implicit receiver";
                    queueManualFixitWarning(s, msg);
                    return {};
                }
            }

            if (!lastRecordDecl) {
                std::string msg = "Failed to get class name for explicit receiver";
                queueManualFixitWarning(s, msg);
                return {};
            }

            const std::string methodName = signalOrSlotNameFromMacro(s);

            auto methods = Utils::methodsFromString(lastRecordDecl, methodName);
            if (methods.empty()) {
                std::string msg;
                if (isPrivateSlot(methodName)) {
                    msg = "Converting Q_PRIVATE_SLOTS not implemented yet\n";
                } else {
                    if (m_context->isQtDeveloper() && classIsOk(clazy::name(lastRecordDecl))) {
                        // This is OK
                        return {};
                    }
                    msg = "No such method " + methodName + " in class " + lastRecordDecl->getNameAsString();
                }

                queueManualFixitWarning(s, msg);
                return {};
            }
            if (methods.size() != 1) {
                std::string msg = std::string("Too many overloads (") + std::to_string(methods.size()) + std::string(") for method ") + methodName
                    + " for record " + lastRecordDecl->getNameAsString();
                queueManualFixitWarning(s, msg);
                return {};
            } else {
                AccessSpecifierManager *a = m_context->accessSpecifierManager;
                if (!a) {
                    return {};
                }
                const bool isSignal = a->qtAccessSpecifierType(methods[0]) == QtAccessSpecifier_Signal;
                if (isSignal && macroName == "SLOT") {
                    // The method is actually a signal and the user used SLOT()
                    // bail out with the fixing.
                    std::string msg = std::string("Can't fix. SLOT macro used but method " + methodName + " is a signal");
                    queueManualFixitWarning(s, msg);
                    return {};
                }
            }

            auto *methodDecl = methods[0];
            if (methodDecl->isStatic()) {
                return {};
            }

            if (macroNum == 1) {
                // Save the number of parameters of the signal. The slot should not have more arguments.
                senderMethod = methodDecl;
            } else if (macroNum == 2) {
                const unsigned int numReceiverParams = methodDecl->getNumParams();
                if (numReceiverParams > senderMethod->getNumParams()) {
                    std::string msg = std::string("Receiver has more parameters (") + std::to_string(methodDecl->getNumParams()) + ") than signal ("
                        + std::to_string(senderMethod->getNumParams()) + ')';
                    queueManualFixitWarning(s, msg);
                    return {};
                }

                for (unsigned int i = 0; i < numReceiverParams; ++i) {
                    ParmVarDecl *receiverParm = methodDecl->getParamDecl(i);
                    ParmVarDecl *senderParm = senderMethod->getParamDecl(i);
                    if (!clazy::isConvertibleTo(senderParm->getType().getTypePtr(), receiverParm->getType().getTypePtrOrNull())) {
                        std::string msg("Sender's parameters are incompatible with the receiver's");
                        queueManualFixitWarning(s, msg);
                        return {};
                    }
                }
            }

            if ((classification & ConnectFlag_QTimerSingleShot) && methodDecl->getNumParams() > 0) {
                std::string msg = "(QTimer) Fixit not implemented for slot with arguments, use a lambda";
                queueManualFixitWarning(s, msg);
                return {};
            }

            if ((classification & ConnectFlag_QMenuAddAction) && methodDecl->getNumParams() > 0) {
                std::string msg = "(QMenu) Fixit not implemented for slot with arguments, use a lambda";
                queueManualFixitWarning(s, msg);
                return {};
            }

            DeclContext *context = m_context->lastDecl->getDeclContext();

            bool isSpecialProtectedCase = false;
            if (!clazy::canTakeAddressOf(methodDecl, context, /*by-ref*/ isSpecialProtectedCase)) {
                std::string msg = "Can't fix " + clazy::accessString(methodDecl->getAccess()) + ' ' + macroName + ' ' + methodDecl->getQualifiedNameAsString();
                queueManualFixitWarning(s, msg);
                return {};
            }

            std::string qualifiedName;
            auto *contextRecord = clazy::firstContextOfType<CXXRecordDecl>(m_context->lastDecl->getDeclContext());
            const bool isInInclude = sm().getMainFileID() != sm().getFileID(locStart);

            if (isSpecialProtectedCase && contextRecord) {
                // We're inside a derived class trying to take address of a protected base member, must use &Derived::method instead of &Base::method.
                qualifiedName = contextRecord->getNameAsString() + "::" + methodDecl->getNameAsString();
            } else {
                qualifiedName = clazy::getMostNeededQualifiedName(sm(), methodDecl, context, locStart, !isInInclude); // (In includes ignore using directives)
            }

            CharSourceRange expansionRange = sm().getImmediateExpansionRange(s);
            SourceRange range = SourceRange(expansionRange.getBegin(), expansionRange.getEnd());

            const std::string functionPointer = '&' + qualifiedName;
            std::string replacement = functionPointer;

            if ((classification & ConnectFlag_4ArgsConnect) && macroNum == 2) {
                replacement = implicitCallee + ", " + replacement;
            }

            fixits.push_back(FixItHint::CreateReplacement(range, replacement));
            lastRecordDecl = nullptr;
        } else {
            Expr *expr = arg;
            const auto *const record = clazy::getBestDynamicClassType(expr);
            if (record) {
                lastRecordDecl = record;
                if (isQPointer(expr)) {
                    auto endLoc = clazy::locForNextToken(astContext(), arg->getBeginLoc(), tok::comma);
                    if (endLoc.isValid()) {
                        fixits.push_back(FixItHint::CreateInsertion(endLoc, ".data()"));
                    } else {
                        return {};
                    }
                }
            }
        }
    }

    return fixits;
}
