/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015-2017 Sergio Martins <smartins@kde.org>

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

#ifndef CHECK_BASE_H
#define CHECK_BASE_H

#include "clazy_stl.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Parse/Parser.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/PPCallbacks.h>
#include <llvm/Config/llvm-config.h>

#include <string>
#include <utility>
#include <vector>

namespace clang {
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
class ClazyContext;

enum CheckLevel { // See README.md for what each level does
    CheckLevelUndefined = -1,
    CheckLevel0 = 0,
    CheckLevel1,
    CheckLevel2,
    ManualCheckLevel,
    MaxCheckLevel = CheckLevel2,
    DefaultCheckLevel = CheckLevel1
};

class ClazyPreprocessorCallbacks
    : public clang::PPCallbacks
{
public:
    ClazyPreprocessorCallbacks(const ClazyPreprocessorCallbacks &) = delete;
    explicit ClazyPreprocessorCallbacks(CheckBase *check);

    void MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &,
                      clang::SourceRange, const clang::MacroArgs *) override;
    void MacroDefined(const clang::Token &MacroNameTok, const clang::MacroDirective*) override;
    void Defined(const clang::Token &MacroNameTok, const clang::MacroDefinition &, clang::SourceRange Range) override;
    void Ifdef(clang::SourceLocation, const clang::Token &MacroNameTok, const clang::MacroDefinition &) override;
    void Ifndef(clang::SourceLocation Loc, const clang::Token &MacroNameTok, const clang::MacroDefinition &) override;
    void If(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind conditionValue) override;
    void Elif(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind ConditionValue, clang::SourceLocation ifLoc) override;
    void Else(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;
    void Endif(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;
    void InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok, clang::StringRef FileName, bool IsAngled,
                            clang::CharSourceRange FilenameRange, const clang::FileEntry *File, clang::StringRef SearchPath,
                            clang::StringRef RelativePath, const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType) override;
private:
    CheckBase *const check;
};

class ClazyAstMatcherCallback
    : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit ClazyAstMatcherCallback(CheckBase *check);
protected:
    CheckBase *const m_check;
};

class CheckBase
{
public:

    enum Option {
        Option_None = 0,
        Option_CanIgnoreIncludes = 1
    };
    typedef int Options;

    typedef std::vector<CheckBase*> List;
    explicit CheckBase(const std::string &name, const ClazyContext *context,
                       Options = Option_None);
    CheckBase(const CheckBase &other) = delete;

    virtual ~CheckBase();

    std::string name() const { return m_name; }

    void emitWarning(const clang::Decl *, const std::string &error, bool printWarningTag = true);
    void emitWarning(const clang::Stmt *, const std::string &error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, const std::string &error, bool printWarningTag = true);
    void emitWarning(clang::SourceLocation loc, std::string error, const std::vector<clang::FixItHint> &fixits, bool printWarningTag = true);
    void emitInternalError(clang::SourceLocation loc, std::string error);

    virtual void registerASTMatchers(clang::ast_matchers::MatchFinder &) {};

    bool canIgnoreIncludes() const
    {
        return m_options & Option_CanIgnoreIncludes;
    }

    virtual void VisitStmt(clang::Stmt *stm);
    virtual void VisitDecl(clang::Decl *decl);
protected:
    virtual void VisitMacroExpands(const clang::Token &macroNameTok, const clang::SourceRange &, const clang::MacroInfo *minfo = nullptr);
    virtual void VisitMacroDefined(const clang::Token &macroNameTok);
    virtual void VisitDefined(const clang::Token &macroNameTok, const clang::SourceRange &);
    virtual void VisitIfdef(clang::SourceLocation, const clang::Token &);
    virtual void VisitIfndef(clang::SourceLocation, const clang::Token &);
    virtual void VisitIf(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind conditionValue);
    virtual void VisitElif(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind ConditionValue, clang::SourceLocation ifLoc);
    virtual void VisitElse(clang::SourceLocation loc, clang::SourceLocation ifLoc);
    virtual void VisitEndif(clang::SourceLocation loc, clang::SourceLocation ifLoc);
    virtual void VisitInclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok, clang::StringRef FileName, bool IsAngled,
                            clang::CharSourceRange FilenameRange, const clang::FileEntry *File, clang::StringRef SearchPath,
                            clang::StringRef RelativePath, const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType);

    void enablePreProcessorCallbacks();


    bool shouldIgnoreFile(clang::SourceLocation) const;
    void reallyEmitWarning(clang::SourceLocation loc, const std::string &error, const std::vector<clang::FixItHint> &fixits);

    void queueManualFixitWarning(clang::SourceLocation loc, const std::string &message = {});
    bool warningAlreadyEmitted(clang::SourceLocation loc) const;
    bool manualFixitAlreadyQueued(clang::SourceLocation loc) const;
    bool isOptionSet(const std::string &optionName) const;

    // 3 shortcuts for stuff that litter the codebase all over.
    const clang::SourceManager &sm() const { return m_sm; }
    const clang::LangOptions &lo() const { return m_astContext.getLangOpts(); }

    const clang::SourceManager &m_sm;
    const std::string m_name;
    const ClazyContext *const m_context;
    clang::ASTContext &m_astContext;
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
