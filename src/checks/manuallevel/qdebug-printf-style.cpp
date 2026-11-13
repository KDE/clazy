/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qdebug-printf-style.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>

#include <algorithm>
#include <optional>

using namespace clang;

namespace
{
// The printf-style replacement for one operand streamed into QDebug
struct FormatItem {
    std::string format; // The text that goes into the format string, e.g. "%d" or the inlined literal
    std::string arg; // The argument passed for the format specifier, empty if the operand was inlined
};

bool isQDebug(const Expr *expr)
{
    const CXXRecordDecl *record = expr->getType()->getAsCXXRecordDecl();
    return record && clazy::name(record) == "QDebug";
}

// A "<<" call whose left-hand side is a QDebug
CXXOperatorCallExpr *asQDebugStreamOperator(Expr *expr)
{
    auto *op = dyn_cast<CXXOperatorCallExpr>(expr->IgnoreImplicit());
    if (!op || op->getOperator() != OO_LessLess || op->getNumArgs() != 2 || !isQDebug(op->getArg(0))) {
        return nullptr;
    }
    return op;
}

// The streamed-into QMessageLogger::debug()/info()/warning()/critical() call, as created by the
// qDebug()/qCDebug(category) family of macros.
bool isMessageLoggerCall(Expr *expr)
{
    auto *call = dyn_cast<CXXMemberCallExpr>(expr->IgnoreImplicit());
    if (!call || call->getNumArgs() != 0) {
        return false;
    }
    const CXXMethodDecl *method = call->getMethodDecl();
    if (!method || clazy::name(method->getParent()) != "QMessageLogger") {
        return false;
    }
    const StringRef name = clazy::name(method);
    return name == "debug" || name == "info" || name == "warning" || name == "critical";
}

bool isLoggingMacro(StringRef name)
{
    static const StringRef macros[] = {"qDebug", "qInfo", "qWarning", "qCritical"};
    return llvm::is_contained(macros, name);
}

bool isCategoryLoggingMacro(StringRef name)
{
    static const StringRef macros[] = {"qCDebug", "qCInfo", "qCWarning", "qCCritical"};
    return llvm::is_contained(macros, name);
}

// Whether the statement's value is discarded, as opposed to e.g. `auto &d = qDebug() << x;` or `return qDebug() << x;`
bool isDiscardedStatement(const Stmt *stmt, const Stmt *parent)
{
    if (isa<CompoundStmt>(parent)) {
        return true;
    }
    if (const auto *ifStmt = dyn_cast<IfStmt>(parent)) {
        return ifStmt->getThen() == stmt || ifStmt->getElse() == stmt;
    }
    if (const auto *forStmt = dyn_cast<ForStmt>(parent)) {
        return forStmt->getBody() == stmt; // Also the case for the qCDebug() family, which expands to a for loop
    }
    if (const auto *whileStmt = dyn_cast<WhileStmt>(parent)) {
        return whileStmt->getBody() == stmt;
    }
    if (const auto *doStmt = dyn_cast<DoStmt>(parent)) {
        return doStmt->getBody() == stmt;
    }
    if (const auto *switchCase = dyn_cast<SwitchCase>(parent)) {
        return switchCase->getSubStmt() == stmt;
    }
    if (const auto *label = dyn_cast<LabelStmt>(parent)) {
        return label->getSubStmt() == stmt;
    }
    return false;
}

// Strips implicit casts and temporaries. Returns nullptr when the operand goes through a user-defined conversion,
// as printf would then receive the object itself instead of the converted value.
const Expr *operandWithoutImplicitConversions(const Expr *expr)
{
    while (true) {
        if (const auto *cast = dyn_cast<ImplicitCastExpr>(expr)) {
            if (cast->getCastKind() == CK_UserDefinedConversion || cast->getCastKind() == CK_ConstructorConversion) {
                return nullptr;
            }
            expr = cast->getSubExpr();
        } else if (const auto *full = dyn_cast<FullExpr>(expr)) {
            expr = full->getSubExpr();
        } else if (const auto *temporary = dyn_cast<MaterializeTemporaryExpr>(expr)) {
            expr = temporary->getSubExpr();
        } else if (const auto *bind = dyn_cast<CXXBindTemporaryExpr>(expr)) {
            expr = bind->getSubExpr();
        } else {
            return expr;
        }
    }
}

// Calls to anything but const member functions are assumed to change state
bool callsNonConstFunction(const Stmt *stmt)
{
    if (!stmt) {
        return false;
    }
    if (const auto *call = dyn_cast<CallExpr>(stmt)) {
        const auto *method = dyn_cast_or_null<CXXMethodDecl>(call->getDirectCallee());
        if (!method || !method->isConst()) {
            return true;
        }
    }
    return llvm::any_of(stmt->children(), callsNonConstFunction);
}

bool mayChangeState(const Expr *expr, const ASTContext &context)
{
    return expr->HasSideEffects(context, /*IncludePossibleEffects=*/false) || callsNonConstFunction(expr);
}

bool isConstant(const Expr *expr, const ASTContext &context)
{
    const Expr *stripped = expr->IgnoreParenImpCasts();
    return isa<StringLiteral, PredefinedExpr>(stripped) || expr->isEvaluatable(context);
}

// Whether the range contains a comment or a preprocessor directive, which the replacement would drop
bool containsCommentsOrDirectives(CharSourceRange range, const SourceManager &sm, const LangOptions &lo)
{
    // The raw lexer needs the whole null-terminated file buffer, so lex it from the start of the range and stop at its end
    const auto [fileId, beginOffset] = sm.getDecomposedLoc(range.getBegin());
    const unsigned int endOffset = sm.getFileOffset(range.getEnd());
    bool invalid = false;
    const StringRef buffer = sm.getBufferData(fileId, &invalid);
    if (invalid) {
        return true;
    }
    Lexer lexer(sm.getLocForStartOfFile(fileId), lo, buffer.begin(), buffer.begin() + beginOffset, buffer.end());
    lexer.SetCommentRetentionState(true);
    Token token;
    while (!lexer.LexFromRawLexer(token) && sm.getFileOffset(token.getLocation()) < endOffset) {
        if (token.is(tok::comment) || (token.is(tok::hash) && token.isAtStartOfLine())) {
            return true;
        }
    }
    return false;
}

std::string escapePercent(StringRef str)
{
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        result += c;
        if (c == '%') {
            result += '%';
        }
    }
    return result;
}

// Returns the type of the parameter of the operator<< overload which receives the operand
QualType streamedParamType(const CXXOperatorCallExpr *op)
{
    const FunctionDecl *func = op->getDirectCallee();
    if (!func) {
        return {};
    }
    // Member operators take the operand as first parameter, free operators as second one
    const unsigned int index = isa<CXXMethodDecl>(func) ? 0 : 1;
    if (func->getNumParams() <= index) {
        return {};
    }
    return func->getParamDecl(index)->getType().getNonReferenceType().getCanonicalType().getUnqualifiedType();
}

const char *formatSpecifierFor(BuiltinType::Kind kind)
{
    switch (kind) {
    case BuiltinType::Char_S:
    case BuiltinType::Char_U:
        return "%c";
    case BuiltinType::Short:
        return "%hd";
    case BuiltinType::UShort:
        return "%hu";
    case BuiltinType::Int:
        return "%d";
    case BuiltinType::UInt:
        return "%u";
    case BuiltinType::Long:
        return "%ld";
    case BuiltinType::ULong:
        return "%lu";
    case BuiltinType::LongLong:
        return "%lld";
    case BuiltinType::ULongLong:
        return "%llu";
    case BuiltinType::Float:
    case BuiltinType::Double:
        // QDebug prints floating point numbers with QTextStream's defaults, which match %g
        return "%g";
    default:
        return nullptr;
    }
}

bool isCharKind(BuiltinType::Kind kind)
{
    return kind == BuiltinType::Char_S || kind == BuiltinType::Char_U;
}

// Builds the format item for an integer operand. The specifier has to be correct on every platform,
// not just the one clazy runs on. long and long long typedefs such as qsizetype, int64_t or quintptr
// differ between platforms, so they are cast to a fixed type.
std::optional<FormatItem> integerFormatItem(QualType argType, BuiltinType::Kind kind, StringRef operandText)
{
    const char *specifier = formatSpecifierFor(kind);
    if (!specifier) {
        return std::nullopt;
    }
    const bool isLongOrLongLong = kind == BuiltinType::Long || kind == BuiltinType::ULong || kind == BuiltinType::LongLong || kind == BuiltinType::ULongLong;
    const auto *typedefType = argType->getAs<TypedefType>();
    if (!isLongOrLongLong || !typedefType) {
        return FormatItem{specifier, operandText.str()};
    }

    // Only the outermost typedef counts: qsizetype is defined via qint64 on 64-bit platforms, but via qint32 on 32-bit ones
    const StringRef name = typedefType->getDecl()->getName();
    if (name == "qint64" || name == "quint64" || name == "qlonglong" || name == "qulonglong" || name == "ulong") {
        return FormatItem{specifier, operandText.str()}; // Fixed typedefs, the same on every platform
    }
    if (name == "size_t") {
        return FormatItem{"%zu", operandText.str()};
    }
    if (name == "ptrdiff_t") {
        return FormatItem{"%td", operandText.str()};
    }

    const bool isSigned = kind == BuiltinType::Long || kind == BuiltinType::LongLong;
    return FormatItem{isSigned ? "%lld" : "%llu", (isSigned ? "qint64(" : "quint64(") + operandText.str() + ")"};
}
}

QDebugPrintfStyle::QDebugPrintfStyle(const std::string &name, Options options)
    : CheckBase(name, options)
{
}

void QDebugPrintfStyle::VisitStmt(clang::Stmt *stmt)
{
    auto *outermostOp = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!outermostOp || !asQDebugStreamOperator(outermostOp)) {
        return;
    }

    // Only handle whole statements: `qDebug() << a << b;`. This skips the inner operator calls of the
    // chain and expressions whose QDebug result is used further.
    Stmt *child = stmt;
    Stmt *parent = clazy::parent(m_context->parentMap, stmt);
    while (parent && isa<FullExpr>(parent)) {
        child = parent;
        parent = clazy::parent(m_context->parentMap, parent);
    }
    if (!parent || !isDiscardedStatement(child, parent)) {
        return;
    }

    // Collect the streamed operands, the chain is left-associative so walk it from the outside in
    llvm::SmallVector<CXXOperatorCallExpr *, 8> ops;
    Expr *lhs = outermostOp;
    while (auto *op = asQDebugStreamOperator(lhs)) {
        ops.push_back(op);
        lhs = op->getArg(0);
    }
    std::reverse(ops.begin(), ops.end());

    // Anything else in the chain, like .nospace() or .noquote(), changes the output
    if (!isMessageLoggerCall(lhs)) {
        return;
    }

    const SourceManager &sourceManager = sm();
    const SourceLocation loggerLoc = lhs->getBeginLoc();
    if (!loggerLoc.isMacroID()) {
        return;
    }

    // The source text of the macro invocation, e.g. `qDebug` or `qCDebug(category)`.
    // Wrapper macros such as `#define LOG qDebug()` are skipped.
    const CharSourceRange macroRange = sourceManager.getExpansionRange(loggerLoc);
    const StringRef macroName = Lexer::getSourceText(CharSourceRange::getTokenRange(macroRange.getBegin()), sourceManager, lo());
    const bool hasCategory = isCategoryLoggingMacro(macroName);
    if (!hasCategory && !isLoggingMacro(macroName)) {
        return;
    }

    StringRef category;
    if (hasCategory) {
        const StringRef invocation = Lexer::getSourceText(macroRange, sourceManager, lo());
        const size_t openParen = invocation.find('(');
        const size_t closeParen = invocation.rfind(')');
        if (openParen == StringRef::npos || closeParen == StringRef::npos || closeParen <= openParen) {
            return;
        }
        category = invocation.substr(openParen + 1, closeParen - openParen - 1).trim();
        if (category.empty()) {
            return;
        }
    }

    // Stream operands are evaluated left to right, function arguments in unspecified order. Only allow an
    // operand which may change state if all the others are constants, so the order can't matter.
    const ASTContext &context = *astContext();
    const auto statefulOperands = llvm::count_if(ops, [&context](CXXOperatorCallExpr *op) {
        return mayChangeState(op->getArg(1), context);
    });
    if (statefulOperands > 1 || (statefulOperands == 1 && ops.size() > 1 && llvm::any_of(ops, [&context](CXXOperatorCallExpr *op) {
                                     return !isConstant(op->getArg(1), context) && !mayChangeState(op->getArg(1), context);
                                 }))) {
        return;
    }

    llvm::SmallVector<FormatItem, 8> items;
    SourceLocation replacementEnd;
    for (CXXOperatorCallExpr *op : ops) {
        Expr *operand = op->getArg(1);
        const CharSourceRange operandRange = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(operand->getSourceRange()), sourceManager, lo());
        if (operandRange.isInvalid()) {
            return;
        }
        const StringRef operandText = Lexer::getSourceText(operandRange, sourceManager, lo());
        replacementEnd = operandRange.getEnd();

        const QualType paramType = streamedParamType(op);
        if (paramType.isNull()) {
            return;
        }

        // A bit-field is promoted to int when passed to printf, regardless of its declared type
        if (operand->IgnoreImpCasts()->refersToBitField()) {
            return;
        }

        if (const CXXRecordDecl *record = paramType->getAsCXXRecordDecl(); record && clazy::name(record) == "QString") {
            // qUtf16Printable() constructs a QString from its argument, so implicit conversions are fine here.
            // Commas, like in template arguments, would split the macro argument.
            const bool needsParens = operandText.contains(',');
            items.push_back({"%ls", "qUtf16Printable(" + (needsParens ? "(" + operandText.str() + ")" : operandText.str()) + ")"});
            continue;
        }

        if (const BuiltinType *builtin = paramType->getAs<BuiltinType>(); builtin && builtin->getKind() == BuiltinType::Bool) {
            // An operand of << binds tighter than ?:, so it doesn't need parentheses.
            // The condition is converted to bool contextually, like for operator<<(bool).
            items.push_back({"%s", operandText.str() + " ? \"true\" : \"false\""});
            continue;
        }

        // printf receives the operand's own type, not the type of the operator<< parameter it was converted to
        const Expr *arg = operandWithoutImplicitConversions(operand);
        if (!arg) {
            return;
        }
        const QualType argType = arg->getType().getUnqualifiedType();
        const QualType canonicalArgType = argType.getCanonicalType();

        if (paramType->isPointerType() && paramType->getPointeeType().isConstQualified() && paramType->getPointeeType()->isCharType()) {
            // const char *. Inline plain string literals into the format string.
            const bool isCharString =
                (canonicalArgType->isPointerType() || canonicalArgType->isArrayType()) && canonicalArgType->getPointeeOrArrayElementType()->isCharType();
            if (!isCharString) {
                return;
            }
            auto *literal = dyn_cast<StringLiteral>(arg);
            if (literal && literal->isOrdinary() && literal->getString().contains('\0')) {
                return; // printf would stop at the embedded null, unlike QDebug
            }
            if (literal && literal->isOrdinary() && literal->getNumConcatenated() == 1 && literal->getBeginLoc().isFileID() && operandText.size() >= 2
                && operandText.front() == '"' && operandText.back() == '"') {
                const StringRef spelling = operandText.drop_front().drop_back();
                // An escaped '%', such as \x25, would be invisible to escapePercent(), so only inline when the spelling has all of them
                if (spelling.count('%') == literal->getString().count('%')) {
                    items.push_back({escapePercent(spelling), {}});
                    continue;
                }
            }
            items.push_back({"%s", operandText.str()});
            continue;
        }

        const auto *paramBuiltin = paramType->getAs<BuiltinType>();
        const auto *argBuiltin = canonicalArgType->getAs<BuiltinType>();
        if (!paramBuiltin || !argBuiltin) {
            return; // Containers, enums, pointers and other types don't have a printf equivalent
        }

        if (paramType->isIntegerType() && !isCharKind(paramBuiltin->getKind())) {
            // Only accept conversions which don't change the value
            if (!canonicalArgType->isIntegerType() || canonicalArgType->isAnyCharacterType() || canonicalArgType->isBooleanType()
                || context.getTypeSize(canonicalArgType) != context.getTypeSize(paramType)
                || canonicalArgType->isSignedIntegerType() != paramType->isSignedIntegerType()) {
                return;
            }
            std::optional<FormatItem> item = integerFormatItem(argType, argBuiltin->getKind(), operandText);
            if (!item) {
                return;
            }
            items.push_back(std::move(*item));
        } else if (isCharKind(paramBuiltin->getKind())) {
            if (!isCharKind(argBuiltin->getKind())) {
                return;
            }
            items.push_back({"%c", operandText.str()});
        } else if (paramType->isRealFloatingType()) {
            // float is promoted to double, so both work with %g
            if (argBuiltin->getKind() != BuiltinType::Float && argBuiltin->getKind() != BuiltinType::Double) {
                return;
            }
            items.push_back({"%g", operandText.str()});
        } else {
            return;
        }
    }

    const auto replacementRange = CharSourceRange::getCharRange(macroRange.getBegin(), replacementEnd);
    if (containsCommentsOrDirectives(replacementRange, sourceManager, lo())) {
        return;
    }

    // QDebug separates the streamed items by a space
    std::string format;
    std::string args;
    for (const FormatItem &item : items) {
        if (&item != &items.front()) {
            format += ' ';
        }
        format += item.format;
        if (!item.arg.empty()) {
            args += ", " + item.arg;
        }
    }

    std::string replacement = macroName.str() + "(";
    if (!category.empty()) {
        replacement += category.str() + ", ";
    }
    replacement += "\"" + format + "\"" + args + ")";

    emitWarning(loggerLoc,
                "Use " + macroName.str() + "(\"...\") instead of streaming into " + macroName.str() + "(), it generates less code",
                {FixItHint::CreateReplacement(replacementRange, replacement)});
}
