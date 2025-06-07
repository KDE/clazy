/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CHECK_BASE_H
#define CHECK_BASE_H

#include "ClazyContext.h"
#include "clazy_stl.h" // IWYU pragma: keep

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Parse/Parser.h>
#include <llvm/Config/llvm-config.h>

#include <string>
#include <utility>
#include <vector>

namespace clazy
{
using OptionalFileEntryRef = clang::CustomizableOptional<clang::FileEntryRef>;
}

namespace clang
{
class CXXMethodDecl;
class Stmt;
class Decl;
class TranslationUnitDecl;
class FixItHint;
class PresumedLoc;
class SourceLocation;
class PreprocessorOptions;
class LangOptions;
class MacroArgs;
class MacroDefinition;
class MacroDirective;
class MacroInfo;
class SourceManager;
class Token;
}

class CheckBase;

enum CheckLevel { // See README.md for what each level does
    CheckLevelUndefined = -1,
    CheckLevel0 = 0,
    CheckLevel1,
    CheckLevel2,
    ManualCheckLevel,
    MaxCheckLevel = CheckLevel2,
    DefaultCheckLevel = CheckLevel1
};

class ClazyPreprocessorCallbacks : public clang::PPCallbacks
{
public:
    ClazyPreprocessorCallbacks(const ClazyPreprocessorCallbacks &) = delete;
    explicit ClazyPreprocessorCallbacks(CheckBase *check);

    void MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &, clang::SourceRange, const clang::MacroArgs *) override;
    void MacroDefined(const clang::Token &MacroNameTok, const clang::MacroDirective *) override;
    void Defined(const clang::Token &MacroNameTok, const clang::MacroDefinition &, clang::SourceRange Range) override;
    void Ifdef(clang::SourceLocation, const clang::Token &MacroNameTok, const clang::MacroDefinition &) override;
    void Ifndef(clang::SourceLocation Loc, const clang::Token &MacroNameTok, const clang::MacroDefinition &) override;
    void If(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind conditionValue) override;
    void Elif(clang::SourceLocation loc,
              clang::SourceRange conditionRange,
              clang::PPCallbacks::ConditionValueKind ConditionValue,
              clang::SourceLocation ifLoc) override;
    void Else(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;
    void Endif(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;
    void InclusionDirective(clang::SourceLocation HashLoc,
                            const clang::Token &IncludeTok,
                            llvm::StringRef FileName,
                            bool IsAngled,
                            clang::CharSourceRange FilenameRange,
                            clazy::OptionalFileEntryRef File,
                            llvm::StringRef SearchPath,
                            llvm::StringRef RelativePath,
                            const clang::Module *SuggestedModule,
                            bool ModuleImported,
                            clang::SrcMgr::CharacteristicKind FileType) override;

private:
    CheckBase *const check;
};

class ClazyAstMatcherCallback : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit ClazyAstMatcherCallback(CheckBase *check);

protected:
    CheckBase *const m_check;
};

class CheckBase
{
public:
    enum Option { Option_None = 0, Option_CanIgnoreIncludes = 1 };
    using Options = int;

    using List = std::vector<CheckBase *>;
    explicit CheckBase(const std::string &name, const ClazyContext *context, Options = Option_None);
    CheckBase(const CheckBase &other) = delete;

    virtual ~CheckBase();

    std::string name() const
    {
        return m_name;
    }

    void emitWarning(const clang::Decl *, const std::string &error, bool printWarningTag = true);
    void emitWarning(const clang::Stmt *, const std::string &error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, const std::string &error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, std::string error, const std::vector<clang::FixItHint> &fixits, bool printWarningTag = true);
    void emitInternalError(clang::SourceLocation loc, std::string error);

    virtual void registerASTMatchers(clang::ast_matchers::MatchFinder &)
    {
    }

    bool canIgnoreIncludes() const
    {
        return m_options & Option_CanIgnoreIncludes;
    }

    virtual void VisitStmt(clang::Stmt *stm);
    virtual void VisitDecl(clang::Decl *decl);

    void enablePreProcessorCallbacks(clang::Preprocessor &pp);
    // To be initialized later
    const ClazyContext *m_context;

protected:
    virtual void VisitMacroExpands(const clang::Token &macroNameTok, const clang::SourceRange &, const clang::MacroInfo *minfo = nullptr);
    virtual void VisitMacroDefined(const clang::Token &macroNameTok);
    virtual void VisitDefined(const clang::Token &macroNameTok, const clang::SourceRange &);
    virtual void VisitIfdef(clang::SourceLocation, const clang::Token &);
    virtual void VisitIfndef(clang::SourceLocation, const clang::Token &);
    virtual void VisitIf(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind conditionValue);
    virtual void
    VisitElif(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind ConditionValue, clang::SourceLocation ifLoc);
    virtual void VisitElse(clang::SourceLocation loc, clang::SourceLocation ifLoc);
    virtual void VisitEndif(clang::SourceLocation loc, clang::SourceLocation ifLoc);
    virtual void VisitInclusionDirective(clang::SourceLocation HashLoc,
                                         const clang::Token &IncludeTok,
                                         llvm::StringRef FileName,
                                         bool IsAngled,
                                         clang::CharSourceRange FilenameRange,
                                         clazy::OptionalFileEntryRef File,
                                         llvm::StringRef SearchPath,
                                         llvm::StringRef RelativePath,
                                         const clang::Module *SuggestedModule,
                                         bool ModuleImported,
                                         clang::SrcMgr::CharacteristicKind FileType);

    bool shouldIgnoreFile(clang::SourceLocation) const;
    void reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const std::vector<clang::FixItHint> &fixits);

    void queueManualFixitWarning(clang::SourceLocation loc, const std::string &message = {});
    bool warningAlreadyEmitted(clang::SourceLocation loc) const;
    bool manualFixitAlreadyQueued(clang::SourceLocation loc) const;
    bool isOptionSet(const std::string &optionName) const;

    // 3 shortcuts for stuff that litter the codebase all over.
    const clang::SourceManager &sm() const
    {
        return m_context->sm;
    }
    const clang::LangOptions &lo() const
    {
        return m_context->astContext.getLangOpts();
    }

    clang::ASTContext *astContext() const
    {
        return &m_context->astContext;
    }

    const std::string m_name;
    std::vector<std::string> m_filesToIgnore;

private:
    friend class ClazyPreprocessorCallbacks;
    friend class ClazyAstMatcherCallback;
    ClazyPreprocessorCallbacks *const m_preprocessorCallbacks;
    std::vector<unsigned int> m_emittedWarningsInMacro;
    std::vector<unsigned int> m_emittedManualFixItsWarningsInMacro;
    std::vector<std::pair<clang::SourceLocation, std::string>> m_queuedManualInterventionWarnings;
    const Options m_options;
    const std::string m_tag;
};

#endif
