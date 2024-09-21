/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qcolor-from-literal.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"

#include <cctype>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace clang::ast_matchers;

static bool isSingleDigitRgb(llvm::StringRef ref)
{
    return ref.size() == 4;
}
static bool isDoubleDigitRgb(llvm::StringRef ref)
{
    return ref.size() == 7;
}
static bool isDoubleDigitRgba(llvm::StringRef ref)
{
    return ref.size() == 9;
}
static bool isTripleDigitRgb(llvm::StringRef ref)
{
    return ref.size() == 10;
}
static bool isQuadrupleDigitRgb(llvm::StringRef ref)
{
    return ref.size() == 13;
}

static bool isStringColorLiteralPattern(StringRef str)
{
    if (!clazy::startsWith(str, "#")) {
        return false;
    }
    return isSingleDigitRgb(str) || isDoubleDigitRgb(str) || isDoubleDigitRgba(str) || isTripleDigitRgb(str) || isQuadrupleDigitRgb(str);
}

class QColorFromLiteral_Callback : public ClazyAstMatcherCallback
{
public:
    using ClazyAstMatcherCallback::ClazyAstMatcherCallback;

    void run(const MatchFinder::MatchResult &result) override
    {
        auto *lt = result.Nodes.getNodeAs<StringLiteral>("myLiteral");
        const Expr *replaceExpr = lt; // When QColor::fromString is used, we want to wrap the QColor constructor around it
        bool isStaticFromString = false;
        if (auto res = result.Nodes.getNodeAs<CallExpr>("methodCall"); res && res->getNumArgs() == 1) {
            if (Expr *argExpr = const_cast<Expr *>(res->getArg(0))) {
                lt = clazy::getFirstChildOfType<StringLiteral>(argExpr);
                replaceExpr = res;
                isStaticFromString = true;
            }
        }
        if (!lt) {
            return;
        }

        llvm::StringRef str = lt->getString();
        if (!clazy::startsWith(str, "#")) {
            return;
        }

        const bool singleDigit = isSingleDigitRgb(str);
        const bool doubleDigit = isDoubleDigitRgb(str);
        const bool doubleDigitA = isDoubleDigitRgba(str);
        if (bool isAnyValidPattern = singleDigit || doubleDigit || doubleDigitA || isTripleDigitRgb(str) || isQuadrupleDigitRgb(str); !isAnyValidPattern) {
            m_check->emitWarning(replaceExpr->getBeginLoc(), "Pattern length does not match any supported one by QColor, check the documentation");
            return;
        }

        for (unsigned int i = 1; i < str.size(); ++i) {
            if (!isxdigit(str[i])) {
                m_check->emitWarning(replaceExpr->getBeginLoc(), "QColor pattern may only contain hexadecimal digits");
                return;
            }
        }

        if (singleDigit || doubleDigit || doubleDigitA) {
            const int increment = singleDigit ? 1 : 2;
            int endPos = 1;
            int startPos = 1;

            std::string aColor = doubleDigitA ? getHexValue(str, startPos, endPos, increment) : "";
            std::string rColor = getHexValue(str, startPos, endPos, increment);
            std::string gColor = getHexValue(str, startPos, endPos, increment);
            std::string bColor = getHexValue(str, startPos, endPos, increment);

            std::string fixit;
            std::string message;
            if (doubleDigitA) {
                const static std::string sep = ", ";
                fixit = prefixHex(rColor) + sep + prefixHex(gColor) + sep + prefixHex(bColor) + sep + prefixHex(aColor);
                message = "The QColor ctor taking ints is cheaper than one taking string literals";
            } else {
                fixit = "0x" + twoDigit(rColor) + twoDigit(gColor) + twoDigit(bColor);
                message = "The QColor ctor taking RGB int value is cheaper than one taking string literals";
            }
            if (isStaticFromString) {
                fixit = "QColor(" + fixit + ")";
            }
            m_check->emitWarning(replaceExpr->getBeginLoc(), message, {clang::FixItHint::CreateReplacement(replaceExpr->getSourceRange(), fixit)});
        } else {
            // triple or quadruple digit RGBA
            m_check->emitWarning(replaceExpr->getBeginLoc(), "The QColor ctor taking QRgba64 is cheaper than one taking string literals");
        }
    }
    inline std::string twoDigit(const std::string &in)
    {
        return in.length() == 1 ? in + in : in;
    }
    inline std::string prefixHex(const std::string &in)
    {
        const static std::string hex = "0x";
        return in == "0" ? in : hex + in;
    }
    inline std::string getHexValue(StringRef fullStr, int &startPos, int &endPos, int increment) const
    {
        endPos += increment;
        clang::StringRef color = fullStr.slice(startPos, endPos);
        startPos = endPos;

        int result = 0;
        color.getAsInteger(16, result);
        if (result == 0) {
            return "0";
        } else {
            return color.str();
        }
    }
};

QColorFromLiteral::QColorFromLiteral(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
    , m_astMatcherCallBack(new QColorFromLiteral_Callback(this))
{
}

QColorFromLiteral::~QColorFromLiteral()
{
    delete m_astMatcherCallBack;
}

void QColorFromLiteral::VisitStmt(Stmt *stmt)
{
    auto *call = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!call || call->getNumArgs() != 1) {
        return;
    }

    std::string name = clazy::qualifiedMethodName(call);
    if (name != "QColor::setNamedColor") {
        return;
    }

    auto *lt = clazy::getFirstChildOfType2<StringLiteral>(call->getArg(0));
    if (lt && isStringColorLiteralPattern(lt->getString())) {
        emitWarning(lt, "The ctor taking ints is cheaper than QColor::setNamedColor(QString)");
    }
}

void QColorFromLiteral::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxConstructExpr(hasDeclaration(namedDecl(hasName("QColor"))), hasArgument(0, stringLiteral().bind("myLiteral"))), m_astMatcherCallBack);
    finder.addMatcher(callExpr(hasDeclaration(cxxMethodDecl(hasName("fromString"), hasParent(cxxRecordDecl(hasName("QColor")))))).bind("methodCall"),
                      m_astMatcherCallBack);
}
