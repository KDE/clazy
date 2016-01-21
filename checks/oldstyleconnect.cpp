/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "oldstyleconnect.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "FixItUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>
#include <clang/Parse/Parser.h>

#include <regex>

using namespace clang;
using namespace std;


enum Fixit {
    FixitNone = 0,
    FixItConnects = 1
};

enum ConnectFlag {
    ConnectFlag_None = 0,       // Not a disconnect or connect
    ConnectFlag_Connect = 1,    // It's a connect
    ConnectFlag_Disconnect = 2, // It's a disconnect
    ConnectFlag_QTimerSingleShot = 4,
    ConnectFlag_OldStyle = 8,   // Qt4 style
    ConnectFlag_4ArgsDisconnect = 16 , // disconnect(const char *signal = 0, const QObject *receiver = 0, const char *method = 0) const
    ConnectFlag_2ArgsDisconnect = 32, //disconnect(const QObject *receiver, const char *method = 0) const
    ConnectFlag_5ArgsConnect = 64, // connect(const QObject *sender, const char *signal, const QObject *receiver, const char *method, Qt::ConnectionType type = Qt::AutoConnection)
    ConnectFlag_4ArgsConnect = 128, // connect(const QObject *sender, const char *signal, const char *method, Qt::ConnectionType type = Qt::AutoConnection)
    ConnectFlag_OldStyleButNonLiteral = 256, // connect(foo, SIGNAL(bar()), foo, variableWithSlotName); // here the slot name isn't a literal
    ConnectFlag_QStateAddTransition = 512,
    ConnectFlag_Bogus = 1024
};

static bool classIsOk(const string &className)
{
    // List of classes we usually use Qt4 syntax
    static const vector<string> okClasses = { "QDBusInterface" };
    return std::find(okClasses.cbegin(), okClasses.cend(), className) != okClasses.cend();
}

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
class PreprocessorCallbacks : public clang::PPCallbacks
{
public:

    PreprocessorCallbacks(OldStyleConnect *q, const SourceManager &sm, const LangOptions &lo)
        : clang::PPCallbacks()
        , q(q)
        , m_sm(sm)
        , m_langOpts(lo)

    {
    }

    void MacroExpands (const Token &MacroNameTok, const MacroDefinition &MD, SourceRange range, const MacroArgs *Args) override
    {
        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (!ii || ii->getName() != "Q_PRIVATE_SLOT")
            return;

        auto charRange = Lexer::getAsCharRange(range, m_sm, m_langOpts);
        const string text = Lexer::getSourceText(charRange, m_sm, m_langOpts);

        static regex rx(R"(Q_PRIVATE_SLOT\s*\((.*)\s*,\s*.*\s+(.*)\(.*)");
        smatch match;
        if (!regex_match(text, match, rx) || match.size() != 3)
            return;

        q->addPrivateSlot({match[1], match[2]});
    }

    OldStyleConnect *const q;
    const SourceManager& m_sm;
    LangOptions m_langOpts;
};
#endif

OldStyleConnect::OldStyleConnect(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
    m_preprocessorCallbacks = new PreprocessorCallbacks(this, m_ci.getSourceManager(), m_ci.getLangOpts());
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
#endif
}

int OldStyleConnect::classifyConnect(FunctionDecl *connectFunc, CallExpr *connectCall)
{
    int classification = ConnectFlag_None;

    const string methodName = connectFunc->getQualifiedNameAsString();
    if (methodName == "QObject::connect")
        classification |= ConnectFlag_Connect;

    if (methodName == "QObject::disconnect")
        classification |= ConnectFlag_Disconnect;

    if (methodName == "QTimer::singleShot")
        classification |= ConnectFlag_QTimerSingleShot;

    if (methodName == "QState::addTransition")
        classification |= ConnectFlag_QStateAddTransition;


    if (classification == ConnectFlag_None)
        return classification;

    // Look for char* arguments
    for (auto it = connectFunc->param_begin(), e = connectFunc->param_end(); it != e; ++it) {
        ParmVarDecl *parm = *it;
        QualType qt = parm->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (!t || !t->isPointerType())
            continue;

        const Type *ptt = t->getPointeeType().getTypePtrOrNull();
        if (ptt && ptt->isCharType()) {
            classification |= ConnectFlag_OldStyle;
            break;
        }
    }

    if (!(classification & ConnectFlag_OldStyle))
        return classification;

    const int numParams = connectFunc->getNumParams();

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
        } else if (numParams == 2) {
            classification |= ConnectFlag_2ArgsDisconnect;
        } else {
            classification |= ConnectFlag_Bogus;
        }
    }

    if (classification & ConnectFlag_OldStyle) {
        // It's old style, but check if all macros are literals
        int numLiterals = 0;
        for (auto it = connectCall->arg_begin(), end = connectCall->arg_end(); it != end; ++it) {
            auto argLocation = (*it)->getLocStart();
            string dummy;
            if (isSignalOrSlot(argLocation, dummy))
                ++numLiterals;
        }

        if ((classification & ConnectFlag_QTimerSingleShot) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if (((classification & ConnectFlag_Connect) && numLiterals != 2)) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_4ArgsDisconnect) && numLiterals != 2)  {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_QStateAddTransition) && numLiterals != 1) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        } else if ((classification & ConnectFlag_Disconnect) && numLiterals == 0) {
            classification |= ConnectFlag_OldStyleButNonLiteral;
        }
    }

    return classification;
}

bool OldStyleConnect::isQPointer(Expr *expr) const
{
    vector<CXXMemberCallExpr*> memberCalls;
    HierarchyUtils::getChilds2<CXXMemberCallExpr>(expr, memberCalls);

    for (auto callExpr : memberCalls) {
        if (!callExpr->getDirectCallee())
            continue;
        CXXMethodDecl *method = dyn_cast<CXXMethodDecl>(callExpr->getDirectCallee());
        if (!method)
            continue;

        // Any better way to detect it's an operator ?
        static regex rx(R"(operator .* \*)");
        if (regex_match(method->getNameAsString(), rx))
            return true;
    }

    return false;
}

bool OldStyleConnect::isPrivateSlot(const string &name) const
{
    return std::find_if(m_privateSlots.cbegin(), m_privateSlots.cend(),
                        [&name](const PrivateSlot &slot){ return slot.name == name; }) != m_privateSlots.cend();
}

void OldStyleConnect::VisitStmt(Stmt *s)
{
    CallExpr *call = dyn_cast<CallExpr>(s);
    if (!call)
        return;

    if (m_lastMethodDecl && m_lastMethodDecl->getParent() && m_lastMethodDecl->getParent()->getNameAsString() == "QObject") // Don't warn of stuff inside qobject.h
        return;

    FunctionDecl *function = call->getDirectCallee();
    if (!function)
        return;

    CXXMethodDecl *method = dyn_cast<CXXMethodDecl>(function);
    if (!method)
        return;

    const int classification = classifyConnect(method, call);
    if (!(classification & ConnectFlag_OldStyle))
        return;

    if ((classification & ConnectFlag_OldStyleButNonLiteral))
        return;

    if (classification & ConnectFlag_Bogus) {
        emitWarning(s->getLocStart(), "Internal error");
        return;
    }

    emitWarning(s->getLocStart(), "Old Style Connect", fixits(classification, call));
}

void OldStyleConnect::addPrivateSlot(const PrivateSlot &slot)
{
    m_privateSlots.push_back(slot);
}

// SIGNAL(foo()) -> foo
string OldStyleConnect::signalOrSlotNameFromMacro(SourceLocation macroLoc)
{
    if (!macroLoc.isMacroID())
        return "error";
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
    auto expansionRange = m_ci.getSourceManager().getImmediateExpansionRange(macroLoc);
    SourceRange range = SourceRange(expansionRange.first, expansionRange.second);
    auto charRange = Lexer::getAsCharRange(range, m_ci.getSourceManager(), m_ci.getLangOpts());
    const string text = Lexer::getSourceText(charRange, m_ci.getSourceManager(), m_ci.getLangOpts());

    static regex rx(R"(\s*(SIGNAL|SLOT)\s*\(\s*(.+)\s*\(.*)");

    smatch match;
    if (regex_match(text, match, rx)) {
        if (match.size() == 3) {
            return match[2].str();
        } else {
            return "error2";
        }
    } else {
        return string("regexp failed for ") + text;
    }
#else
    return {};
#endif
}

bool OldStyleConnect::isSignalOrSlot(SourceLocation loc, string &macroName) const
{
    macroName = {};
    if (!loc.isMacroID() || loc.isInvalid())
        return false;

    macroName = Lexer::getImmediateMacroName(loc, m_ci.getSourceManager(), m_ci.getLangOpts());
    return macroName == "SIGNAL" || macroName == "SLOT";
}

vector<FixItHint> OldStyleConnect::fixits(int classification, CallExpr *call)
{
    if (!isFixitEnabled(FixItConnects))
        return {};

    if (!call) {
        llvm::errs() << "Call is invalid\n";
        return {};
    }

    if (classification & ConnectFlag_2ArgsDisconnect) {
        // Not implemented yet
        string msg = "Fix it not implemented for disconnect with 2 args";
        queueManualFixitWarning(call->getLocStart(), FixItConnects, msg);
        return {};
    }

    vector<FixItHint> fixits;
    int macroNum = 0;
    string implicitCallee;
    string macroName;
    CXXMethodDecl *senderMethod = nullptr;
    for (auto it = call->arg_begin(), end = call->arg_end(); it != end; ++it) {
        auto s = (*it)->getLocStart();
        static const CXXRecordDecl *lastRecordDecl = nullptr;
        if (isSignalOrSlot(s, macroName)) {
            macroNum++;
            if (!lastRecordDecl && (classification & ConnectFlag_4ArgsConnect)) {
                // This means it's a connect with implicit receiver
                lastRecordDecl = Utils::recordForMemberCall(dyn_cast<CXXMemberCallExpr>(call), implicitCallee);

                if (macroNum == 1)
                    llvm::errs() << "This first macro shouldn't enter this path";
                if (!lastRecordDecl) {
                    string msg = "Failed to get class name for implicit receiver";
                    queueManualFixitWarning(s, FixItConnects, msg);
                    return {};
                }
            }

            if (!lastRecordDecl) {
                string msg = "Failed to get class name for explicit receiver";
                queueManualFixitWarning(s, FixItConnects, msg);
                return {};
            }

            const string methodName = signalOrSlotNameFromMacro(s);

            auto methods = Utils::methodsFromString(lastRecordDecl, methodName);
            if (methods.empty()) {
                string msg;
                if (isPrivateSlot(methodName)) {
                    msg = "Converting Q_PRIVATE_SLOTS not implemented yet\n";
                } else {
                    if (classIsOk(lastRecordDecl->getNameAsString())) {
                        // This is OK
                        return {};
                    } else {
                        msg = "No such method " + methodName + " in class " + lastRecordDecl->getNameAsString();
                    }
                }

                queueManualFixitWarning(s, FixItConnects, msg);
                return {};
            } else if (methods.size() != 1) {
                string msg = string("Too many overloads (") + to_string(methods.size()) + string(") for method ")
                             + methodName + " for record " + lastRecordDecl->getNameAsString();
                queueManualFixitWarning(s, FixItConnects, msg);
                return {};
            }

            auto methodDecl = methods[0];
            if (methodDecl->isStatic()) {
                return {};
            }

            if (macroNum == 1) {
                // Save the number of parameters of the signal. The slot should not have more arguments.
                senderMethod = methodDecl;
            } else if (macroNum == 2) {
                const unsigned int numReceiverParams = methodDecl->getNumParams();
                if (numReceiverParams > senderMethod->getNumParams()) {
                    string msg = string("Receiver has more parameters (") + to_string(methodDecl->getNumParams()) + ") than signal (" + to_string(senderMethod->getNumParams()) + ')';
                    queueManualFixitWarning(s, FixItConnects, msg);
                    return {};
                }

                for (unsigned int i = 0; i < numReceiverParams; ++i) {
                    ParmVarDecl *receiverParm = methodDecl->getParamDecl(i);
                    ParmVarDecl *senderParm = senderMethod->getParamDecl(i);
                    if (!Utils::isConvertibleTo(senderParm->getType().getTypePtr(), receiverParm->getType().getTypePtrOrNull())) {
                        string msg = string("Sender's parameters are incompatible with the receiver's");
                        queueManualFixitWarning(s, FixItConnects, msg);
                        return {};
                    }
                }
            }

            if ((classification & ConnectFlag_QTimerSingleShot) && methodDecl->getNumParams() > 0) {
                string msg = "(QTimer) Fixit not implemented for slot with arguments, use a lambda";
                queueManualFixitWarning(s, FixItConnects, msg);
                return {};
            }

            DeclContext *context = m_lastDecl->getDeclContext();

            bool isSpecialProtectedCase = false;
            if (!Utils::canTakeAddressOf(methodDecl, context, /*by-ref*/isSpecialProtectedCase)) {
                string msg = "Can't fix " + StringUtils::accessString(methodDecl->getAccess()) + ' ' + macroName + ' ' + methodDecl->getQualifiedNameAsString();
                queueManualFixitWarning(s, FixItConnects, msg);
                return {};
            }

            string qualifiedName;
            CXXRecordDecl *contextRecord = Utils::firstMethodOrClassContext(m_lastDecl->getDeclContext());
            const bool isInInclude = m_ci.getSourceManager().getMainFileID() != m_ci.getSourceManager().getFileID(call->getLocStart());

            if (isSpecialProtectedCase && contextRecord) {
                // We're inside a derived class trying to take address of a protected base member, must use &Derived::method instead of &Base::method.
                qualifiedName = contextRecord->getNameAsString() + "::" + methodDecl->getNameAsString() ;
            } else {
                qualifiedName = Utils::getMostNeededQualifiedName(m_ci.getSourceManager(), methodDecl, context, call->getLocStart(), !isInInclude); // (In includes ignore using directives)
            }

            auto expansionRange = m_ci.getSourceManager().getImmediateExpansionRange(s);
            SourceRange range = SourceRange(expansionRange.first, expansionRange.second);

            const string functionPointer = '&' + qualifiedName;
            string replacement = functionPointer;

            if ((classification & ConnectFlag_4ArgsConnect) && macroNum == 2)
                replacement = implicitCallee + ", " + replacement;

            fixits.push_back(FixItHint::CreateReplacement(range, replacement));
            lastRecordDecl = nullptr;
        } else {
            Expr *expr = *it;
            const auto record = expr ? expr->getBestDynamicClassType() : nullptr;
            if (record) {
                lastRecordDecl = record;
                if (isQPointer(expr)) {
                    auto endLoc = FixItUtils::locForNextToken(m_ci, (*it)->getLocStart(), tok::comma);
                    if (endLoc.isValid()) {
                        fixits.push_back(FixItHint::CreateInsertion(endLoc, ".data()"));
                    } else {
                        queueManualFixitWarning(s, FixItConnects, "Can't fix this QPointer case");
                        return {};
                    }
                }
            }
        }
    }

    return fixits;
}

const char *const s_checkName = "old-style-connect";
REGISTER_CHECK_WITH_FLAGS(s_checkName, OldStyleConnect, CheckLevel2)
REGISTER_FIXIT(FixItConnects, "fix-old-style-connect", s_checkName)
