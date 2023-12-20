/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qcolor-from-literal.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"

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

// TODO: setNameFromString()

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

static bool handleStringLiteral(const StringLiteral *literal)
{
    if (!literal) {
        return false;
    }

    llvm::StringRef str = literal->getString();
    if (!str.startswith("#")) {
        return false;
    }

    return isSingleDigitRgb(str) || isDoubleDigitRgb(str) || isDoubleDigitRgba(str) || isTripleDigitRgb(str) || isQuadrupleDigitRgb(str);
}

class QColorFromLiteral_Callback : public ClazyAstMatcherCallback
{
public:
    QColorFromLiteral_Callback(CheckBase *base)
        : ClazyAstMatcherCallback(base)
    {
    }

    void run(const MatchFinder::MatchResult &result) override
    {
        const auto *lt = result.Nodes.getNodeAs<StringLiteral>("myLiteral");
        if (!handleStringLiteral(lt)) {
            return;
        }
        llvm::StringRef str = lt->getString();
        const bool singleDigit = isSingleDigitRgb(str);
        const bool doubleDigit = isDoubleDigitRgb(str);
        const bool doubleDigitA = isDoubleDigitRgba(str);
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
            m_check->emitWarning(clazy::getLocStart(lt), message, {clang::FixItHint::CreateReplacement(lt->getSourceRange(), fixit)});
        } else {
            // triple or quadruple digit RGBA
            m_check->emitWarning(clazy::getLocStart(lt), "The QColor ctor taking QRgba64 is cheaper than one taking string literals");
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
    if (handleStringLiteral(lt)) {
        emitWarning(lt, "The ctor taking ints is cheaper than QColor::setNamedColor(QString)");
    }
}

void QColorFromLiteral::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxConstructExpr(hasDeclaration(namedDecl(hasName("QColor"))), hasArgument(0, stringLiteral().bind("myLiteral"))), m_astMatcherCallBack);
}
