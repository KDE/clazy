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
#include "checks/manuallevel/container-inside-loop.h"
#include "checks/manuallevel/inefficientqlist.h"
#include "checks/manuallevel/isempty-vs-count.h"
#include "checks/manuallevel/qstring-varargs.h"
#include "checks/manuallevel/qt4-qstring-from-array.h"
#include "checks/manuallevel/tr-non-literal.h"
#include "checks/manuallevel/unneeded-cast.h"
#include "checks/level0/connect-by-name.h"
#include "checks/level0/connect-non-signal.h"
#include "checks/level0/connect-not-normalized.h"
#include "checks/level0/container-anti-pattern.h"
#include "checks/level0/fully-qualified-moc-types.h"
#include "checks/level0/lambda-in-connect.h"
#include "checks/level0/lambda-unique-connection.h"
#include "checks/level0/mutable-container-key.h"
#include "checks/level0/qcolor-from-literal.h"
#include "checks/level0/qdatetimeutc.h"
#include "checks/level0/qenums.h"
#include "checks/level0/qfileinfo-exists.h"
#include "checks/level0/qgetenv.h"
#include "checks/level0/qmap-with-pointer-key.h"
#include "checks/level0/qstringarg.h"
#include "checks/level0/qstring-insensitive-allocation.h"
#include "checks/level0/qstringref.h"
#include "checks/level0/qt-macros.h"
#include "checks/level0/qvariant-template-instantiation.h"
#include "checks/level0/strict-iterators.h"
#include "checks/level0/temporaryiterator.h"
#include "checks/level0/unused-non-trivial-variable.h"
#include "checks/level0/writingtotemporary.h"
#include "checks/level0/wrong-qglobalstatic.h"
#include "checks/level1/autounexpectedqstringbuilder.h"
#include "checks/level1/child-event-qobject-cast.h"
#include "checks/level1/connect-3arg-lambda.h"
#include "checks/level1/const-signal-or-slot.h"
#include "checks/level1/detachingtemporary.h"
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
#include "checks/level1/ruleoftwosoft.h"
#include "checks/level1/skipped-base-method.h"
#include "checks/level1/virtual-signal.h"
#include "checks/level2/base-class-event.h"
#include "checks/level2/copyable-polymorphic.h"
#include "checks/level2/ctor-missing-parent-argument.h"
#include "checks/level2/function-args-by-ref.h"
#include "checks/level2/function-args-by-value.h"
#include "checks/level2/globalconstcharpointer.h"
#include "checks/level2/implicitcasts.h"
#include "checks/level2/missing-qobject-macro.h"
#include "checks/level2/missing-typeinfo.h"
#include "checks/level2/oldstyleconnect.h"
#include "checks/level2/qstring-allocations.h"
#include "checks/level2/reservecandidates.h"
#include "checks/level2/returning-void-expression.h"
#include "checks/level2/ruleofthree.h"
#include "checks/level2/virtual-call-ctor.h"
#include "checks/level3/assertwithsideeffects.h"
#include "checks/level3/detachingmember.h"
#include "checks/level3/thread-with-slots.h"

template <typename T>
RegisteredCheck check(const char *name, CheckLevel level, RegisteredCheck::Options options = RegisteredCheck::Option_None)
{
    auto factoryFuntion = [name](ClazyContext *context){ return new T(name, context); };
    return RegisteredCheck{name, level, factoryFuntion, options};
}

void CheckManager::registerChecks()
{
    registerCheck(check<ContainerInsideLoop>("container-inside-loop", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<InefficientQList>("inefficient-qlist", ManualCheckLevel,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<IsEmptyVSCount>("isempty-vs-count", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QStringVarargs>("qstring-varargs", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<Qt4QStringFromArray>("qt4-qstring-from-array", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qt4-qstring-from-array", "qt4-qstring-from-array");
    registerCheck(check<TrNonLiteral>("tr-non-literal", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<UnneededCast>("unneeded-cast", ManualCheckLevel,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ConnectByName>("connect-by-name", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<ConnectNonSignal>("connect-non-signal", CheckLevel0, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ConnectNotNormalized>("connect-not-normalized", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ContainerAntiPattern>("container-anti-pattern", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<FullyQualifiedMocTypes>("fully-qualified-moc-types", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<LambdaInConnect>("lambda-in-connect", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<LambdaUniqueConnection>("lambda-unique-connection", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<MutableContainerKey>("mutable-container-key", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QColorFromLiteral>("qcolor-from-literal", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QDateTimeUtc>("qdatetime-utc", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qdatetime-utc", "qdatetime-utc");
    registerCheck(check<QEnums>("qenums", CheckLevel0, RegisteredCheck::Option_Qt4Incompatible));
    registerCheck(check<QFileInfoExists>("qfileinfo-exists", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QGetEnv>("qgetenv", CheckLevel0, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qgetenv", "qgetenv");
    registerCheck(check<QMapWithPointerKey>("qmap-with-pointer-key", CheckLevel0,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<QStringArg>("qstring-arg", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<QStringInsensitiveAllocation>("qstring-insensitive-allocation", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<StringRefCandidates>("qstring-ref", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-missing-qstringref", "qstring-ref");
    registerCheck(check<QtMacros>("qt-macros", CheckLevel0, RegisteredCheck::Option_None));
    registerCheck(check<QVariantTemplateInstantiation>("qvariant-template-instantiation", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<StrictIterators>("strict-iterators", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<TemporaryIterator>("temporary-iterator", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<UnusedNonTrivialVariable>("unused-non-trivial-variable", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<WritingToTemporary>("writing-to-temporary", CheckLevel0,  RegisteredCheck::Option_VisitsStmts));
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
    registerCheck(check<ReturningDataFromTemporary>("returning-data-from-temporary", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<RuleOfTwoSoft>("rule-of-two-soft", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<SkippedBaseMethod>("skipped-base-method", CheckLevel1,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<VirtualSignal>("virtual-signal", CheckLevel1,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<BaseClassEvent>("base-class-event", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<CopyablePolymorphic>("copyable-polymorphic", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<CtorMissingParentArgument>("ctor-missing-parent-argument", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<FunctionArgsByRef>("function-args-by-ref", CheckLevel2,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<FunctionArgsByValue>("function-args-by-value", CheckLevel2,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<GlobalConstCharPointer>("global-const-char-pointer", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<ImplicitCasts>("implicit-casts", CheckLevel2,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<MissingQObjectMacro>("missing-qobject-macro", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<MissingTypeInfo>("missing-typeinfo", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
#ifndef NO_STD_REGEX
    registerCheck(check<OldStyleConnect>("old-style-connect", CheckLevel2, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-old-style-connect", "old-style-connect");
#endif
    registerCheck(check<QStringAllocations>("qstring-allocations", CheckLevel2, RegisteredCheck::Option_Qt4Incompatible | RegisteredCheck::Option_VisitsStmts));
    registerFixIt(1, "fix-qlatin1string-allocations", "qstring-allocations");
    registerFixIt(2, "fix-fromLatin1_fromUtf8-allocations", "qstring-allocations");
    registerFixIt(4, "fix-fromCharPtrAllocations", "qstring-allocations");
    registerCheck(check<ReserveCandidates>("reserve-candidates", CheckLevel2,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ReturningVoidExpression>("returning-void-expression", CheckLevel2,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<RuleOfThree>("rule-of-three", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<VirtualCallCtor>("virtual-call-ctor", CheckLevel2,  RegisteredCheck::Option_VisitsDecls));
    registerCheck(check<AssertWithSideEffects>("assert-with-side-effects", CheckLevel3,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<DetachingMember>("detaching-member", CheckLevel3,  RegisteredCheck::Option_VisitsStmts));
    registerCheck(check<ThreadWithSlots>("thread-with-slots", CheckLevel3,  RegisteredCheck::Option_VisitsStmts | RegisteredCheck::Option_VisitsDecls));
}
