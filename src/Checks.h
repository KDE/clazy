/*
  This file is part of the clazy static checker.

  Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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


/**
 * To add a new check you can either edit this file, or use the python script:
 * dev-scripts/generate.py > src/Checks.h
 */

#include "checkmanager.h"
#include "checks/manuallevel/assert-with-side-effects.h"
#include "checks/manuallevel/container-inside-loop.h"
#include "checks/manuallevel/detaching-member.h"
#include "checks/manuallevel/heap-allocated-small-trivial-type.h"
#include "checks/manuallevel/ifndef-define-typo.h"
#include "checks/manuallevel/inefficient-qlist.h"
#include "checks/manuallevel/isempty-vs-count.h"
#include "checks/manuallevel/jnisignatures.h"
#include "checks/manuallevel/qhash-with-char-pointer-key.h"
#include "checks/manuallevel/qproperty-type-mismatch.h"
#include "checks/manuallevel/qrequiredresult-candidates.h"
#include "checks/manuallevel/qstring-varargs.h"
#include "checks/manuallevel/qt-keywords.h"
#include "checks/manuallevel/qt4-qstring-from-array.h"
#include "checks/manuallevel/qt6-header-fixes.h"
#include "checks/manuallevel/qt6-qdir-fixes.h"
#include "checks/manuallevel/qt6-qhash-signature.h"
#include "checks/manuallevel/qt6-qlatin1stringchar-to-u.h"
#include "checks/manuallevel/qvariant-template-instantiation.h"
#include "checks/manuallevel/raw-environment-function.h"
#include "checks/manuallevel/reserve-candidates.h"
#include "checks/manuallevel/signal-with-return-value.h"
#include "checks/manuallevel/thread-with-slots.h"
#include "checks/manuallevel/tr-non-literal.h"
#include "checks/manuallevel/unneeded-cast.h"
#include "checks/manuallevel/use-chrono-in-qtimer.h"
#include "checks/level0/connect-by-name.h"
#include "checks/level0/connect-non-signal.h"
#include "checks/level0/connect-not-normalized.h"
#include "checks/level0/container-anti-pattern.h"
#include "checks/level0/empty-qstringliteral.h"
#include "checks/level0/fully-qualified-moc-types.h"
#include "checks/level0/lambda-in-connect.h"
#include "checks/level0/lambda-unique-connection.h"
#include "checks/level0/lowercase-qml-type-name.h"
#include "checks/level0/mutable-container-key.h"
#include "checks/level0/overloaded-signal.h"
#include "checks/level0/qcolor-from-literal.h"
#include "checks/level0/qdatetime-utc.h"
#include "checks/level0/qenums.h"
#include "checks/level0/qfileinfo-exists.h"
#include "checks/level0/qgetenv.h"
#include "checks/level0/qmap-with-pointer-key.h"
#include "checks/level0/qstring-arg.h"
#include "checks/level0/qstring-comparison-to-implicit-char.h"
#include "checks/level0/qstring-insensitive-allocation.h"
#include "checks/level0/qstring-ref.h"
#include "checks/level0/qt-macros.h"
#include "checks/level0/strict-iterators.h"
#include "checks/level0/temporary-iterator.h"
#include "checks/level0/unused-non-trivial-variable.h"
#include "checks/level0/writing-to-temporary.h"
#include "checks/level0/wrong-qevent-cast.h"
#include "checks/level0/wrong-qglobalstatic.h"
#include "checks/level1/auto-unexpected-qstringbuilder.h"
#include "checks/level1/child-event-qobject-cast.h"
#include "checks/level1/connect-3arg-lambda.h"
#include "checks/level1/const-signal-or-slot.h"
#include "checks/level1/detaching-temporary.h"
#include "checks/level1/foreach.h"
#include "checks/level1/incorrect-emit.h"
#include "checks/level1/inefficient-qlist-soft.h"
#include "checks/level1/install-event-filter.h"
#include "checks/level1/non-pod-global-static.h"
#include "checks/level1/overridden-signal.h"
#include "checks/level1/post-event.h"
#include "checks/level1/qdeleteall.h"
#include "checks/level1/qhash-namespace.h"
#include "checks/level1/qlatin1string-non-ascii.h"
#include "checks/level1/qproperty-without-notify.h"
#include "checks/level1/qstring-left.h"
#include "checks/level1/range-loop.h"
#include "checks/level1/returning-data-from-temporary.h"
#include "checks/level1/rule-of-two-soft.h"
#include "checks/level1/skipped-base-method.h"
#include "checks/level1/virtual-signal.h"
#include "checks/level2/base-class-event.h"
#include "checks/level2/copyable-polymorphic.h"
#include "checks/level2/ctor-missing-parent-argument.h"
#include "checks/level2/function-args-by-ref.h"
#include "checks/level2/function-args-by-value.h"
#include "checks/level2/global-const-char-pointer.h"
#include "checks/level2/implicit-casts.h"
#include "checks/level2/missing-qobject-macro.h"
#include "checks/level2/missing-typeinfo.h"
#include "checks/level2/old-style-connect.h"
#include "checks/level2/qstring-allocations.h"
#include "checks/level2/returning-void-expression.h"
#include "checks/level2/rule-of-three.h"
#include "checks/level2/static-pmf.h"
#include "checks/level2/virtual-call-ctor.h"

template <typename T>
RegisteredCheck check(const char *name, CheckLevel level, RegisteredCheck::Options options = RegisteredCheck::Option_None)
{
    auto factoryFuntion = [name](ClazyContext *context){ return new T(name, context); };
    return RegisteredCheck{name, level, factoryFuntion, options};
}

void CheckManager::registerChecks()
{
    registerCheck(check<AssertWithSideEffects>("assert-with-side-effects", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ContainerInsideLoop>("container-inside-loop", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<DetachingMember>("detaching-member", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<HeapAllocatedSmallTrivialType>("heap-allocated-small-trivial-type", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<IfndefDefineTypo>("ifndef-define-typo", ManualCheckLevel, RegisteredCheck::Option_None));
    registerCheck(check<InefficientQList>("inefficient-qlist", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<IsEmptyVSCount>("isempty-vs-count", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<JniSignatures>("jni-signatures", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QHashWithCharPointerKey>("qhash-with-char-pointer-key", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QPropertyTypeMismatch>("qproperty-type-mismatch", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QRequiredResultCandidates>("qrequiredresult-candidates", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QStringVarargs>("qstring-varargs", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QtKeywords>("qt-keywords", ManualCheckLevel, RegisteredCheck::Option_None));
    registerFixIt(1, "fix-qt-keywords", "qt-keywords");
    registerCheck(check<Qt4QStringFromArray>("qt4-qstring-from-array", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qt4-qstring-from-array", "qt4-qstring-from-array");
    registerCheck(check<Qt6HeaderFixes>("qt6-header-fixes", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qt6-header-fixes", "qt6-header-fixes");
    registerCheck(check<Qt6QDirFixes>("qt6-qdir-fixes", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qt6-qdir-fixes", "qt6-qdir-fixes");
    registerCheck(check<Qt6QHashSignature>("qt6-qhash-signature", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerFixIt(1, "fix-qt6-qhash-signature", "qt6-qhash-signature");
    registerCheck(check<Qt6QLatin1StringCharToU>("qt6-qlatin1stringchar-to-u", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qt6-qlatin1stringchar-to-u", "qt6-qlatin1stringchar-to-u");
    registerCheck(check<QVariantTemplateInstantiation>("qvariant-template-instantiation", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<RawEnvironmentFunction>("raw-environment-function", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ReserveCandidates>("reserve-candidates", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<SignalWithReturnValue>("signal-with-return-value", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<ThreadWithSlots>("thread-with-slots", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<TrNonLiteral>("tr-non-literal", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<UnneededCast>("unneeded-cast", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<UseChronoInQTimer>("use-chrono-in-qtimer", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ConnectByName>("connect-by-name", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<ConnectNonSignal>("connect-non-signal", CheckLevel0, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ConnectNotNormalized>("connect-not-normalized", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ContainerAntiPattern>("container-anti-pattern", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<EmptyQStringliteral>("empty-qstringliteral", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<FullyQualifiedMocTypes>("fully-qualified-moc-types", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<LambdaInConnect>("lambda-in-connect", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<LambdaUniqueConnection>("lambda-unique-connection", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<LowercaseQMlTypeName>("lowercase-qml-type-name", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<MutableContainerKey>("mutable-container-key", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<OverloadedSignal>("overloaded-signal", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
#ifndef CLAZY_DISABLE_AST_MATCHERS
    registerCheck(check<QColorFromLiteral>("qcolor-from-literal", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
#endif
    registerCheck(check<QDateTimeUtc>("qdatetime-utc", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qdatetime-utc", "qdatetime-utc");
    registerCheck(check<QEnums>("qenums", CheckLevel0, RegisteredCheck::Option_Qt4Incompatible));
    registerCheck(check<QFileInfoExists>("qfileinfo-exists", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QGetEnv>("qgetenv", CheckLevel0, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qgetenv", "qgetenv");
    registerCheck(check<QMapWithPointerKey>("qmap-with-pointer-key", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QStringArg>("qstring-arg", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QStringComparisonToImplicitChar>("qstring-comparison-to-implicit-char", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QStringInsensitiveAllocation>("qstring-insensitive-allocation", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<StringRefCandidates>("qstring-ref", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-missing-qstringref", "qstring-ref");
    registerCheck(check<QtMacros>("qt-macros", CheckLevel0, RegisteredCheck::Option_None));
    registerCheck(check<StrictIterators>("strict-iterators", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<TemporaryIterator>("temporary-iterator", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<UnusedNonTrivialVariable>("unused-non-trivial-variable", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<WritingToTemporary>("writing-to-temporary", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<WrongQEventCast>("wrong-qevent-cast", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<WrongQGlobalStatic>("wrong-qglobalstatic", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<AutoUnexpectedQStringBuilder>("auto-unexpected-qstringbuilder", CheckLevel1,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerFixIt(1, "fix-auto-unexpected-qstringbuilder", "auto-unexpected-qstringbuilder");
    registerCheck(check<ChildEventQObjectCast>("child-event-qobject-cast", CheckLevel1,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<Connect3ArgLambda>("connect-3arg-lambda", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ConstSignalOrSlot>("const-signal-or-slot", CheckLevel1,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<DetachingTemporary>("detaching-temporary", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<Foreach>("foreach", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<IncorrectEmit>("incorrect-emit", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<InefficientQListSoft>("inefficient-qlist-soft", CheckLevel1,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<InstallEventFilter>("install-event-filter", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<NonPodGlobalStatic>("non-pod-global-static", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<OverriddenSignal>("overridden-signal", CheckLevel1,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<PostEvent>("post-event", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QDeleteAll>("qdeleteall", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QHashNamespace>("qhash-namespace", CheckLevel1,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QLatin1StringNonAscii>("qlatin1string-non-ascii", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QPropertyWithoutNotify>("qproperty-without-notify", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QStringLeft>("qstring-left", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<RangeLoop>("range-loop", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-range-loop-add-ref", "range-loop");
    registerFixIt(2, "fix-range-loop-add-qasconst", "range-loop");
    registerCheck(check<ReturningDataFromTemporary>("returning-data-from-temporary", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<RuleOfTwoSoft>("rule-of-two-soft", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<SkippedBaseMethod>("skipped-base-method", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<VirtualSignal>("virtual-signal", CheckLevel1,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<BaseClassEvent>("base-class-event", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<CopyablePolymorphic>("copyable-polymorphic", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<CtorMissingParentArgument>("ctor-missing-parent-argument", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<FunctionArgsByRef>("function-args-by-ref", CheckLevel2,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerFixIt(1, "fix-function-args-by-ref", "function-args-by-ref");
    registerCheck(check<FunctionArgsByValue>("function-args-by-value", CheckLevel2,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<GlobalConstCharPointer>("global-const-char-pointer", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<ImplicitCasts>("implicit-casts", CheckLevel2,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<MissingQObjectMacro>("missing-qobject-macro", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerFixIt(1, "fix-missing-qobject-macro", "missing-qobject-macro");
    registerCheck(check<MissingTypeInfo>("missing-typeinfo", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<OldStyleConnect>("old-style-connect", CheckLevel2, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-old-style-connect", "old-style-connect");
    registerCheck(check<QStringAllocations>("qstring-allocations", CheckLevel2, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qlatin1string-allocations", "qstring-allocations");
    registerFixIt(2, "fix-fromLatin1_fromUtf8-allocations", "qstring-allocations");
    registerFixIt(4, "fix-fromCharPtrAllocations", "qstring-allocations");
    registerCheck(check<ReturningVoidExpression>("returning-void-expression", CheckLevel2,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<RuleOfThree>("rule-of-three", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<StaticPmf>("static-pmf", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<VirtualCallCtor>("virtual-call-ctor", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
}
