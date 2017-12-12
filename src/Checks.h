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

#include "checks/hiddenlevel/inefficientqlist.h"
#include "checks/hiddenlevel/isempty-vs-count.h"
#include "checks/hiddenlevel/qt4-qstring-from-array.h"
#include "checks/hiddenlevel/tr-non-literal.h"
#include "checks/level0/connect-non-signal.h"
#include "checks/level0/lambda-in-connect.h"
#include "checks/level0/lambda-unique-connection.h"
#include "checks/level0/qdatetimeutc.h"
#include "checks/level0/qgetenv.h"
#include "checks/level0/qstring-insensitive-allocation.h"
#include "checks/level0/qvariant-template-instantiation.h"
#include "checks/level0/unused-non-trivial-variable.h"
#include "checks/level0/connect-not-normalized.h"
#include "checks/level0/mutable-container-key.h"
#include "checks/level0/qenums.h"
#include "checks/level0/qmap-with-pointer-key.h"
#include "checks/level0/qstringref.h"
#include "checks/level0/strict-iterators.h"
#include "checks/level0/writingtotemporary.h"
#include "checks/level0/container-anti-pattern.h"
#include "checks/level0/qcolor-from-literal.h"
#include "checks/level0/qfileinfo-exists.h"
#include "checks/level0/qstringarg.h"
#include "checks/level0/qt-macros.h"
#include "checks/level0/temporaryiterator.h"
#include "checks/level0/wrong-qglobalstatic.h"
#include "checks/level1/autounexpectedqstringbuilder.h"
#include "checks/level1/connect-3arg-lambda.h"
#include "checks/level1/const-signal-or-slot.h"
#include "checks/level1/detachingtemporary.h"
#include "checks/level1/foreach.h"
#include "checks/level1/incorrect-emit.h"
#include "checks/level1/inefficient-qlist-soft.h"
#include "checks/level1/install-event-filter.h"
#include "checks/level1/non-pod-global-static.h"
#include "checks/level1/post-event.h"
#include "checks/level1/qdeleteall.h"
#include "checks/level1/qlatin1string-non-ascii.h"
#include "checks/level1/qproperty-without-notify.h"
#include "checks/level1/qstring-left.h"
#include "checks/level1/range-loop.h"
#include "checks/level1/returning-data-from-temporary.h"
#include "checks/level1/ruleoftwosoft.h"
#include "checks/level1/child-event-qobject-cast.h"
#include "checks/level1/virtual-signal.h"
#include "checks/level1/overridden-signal.h"
#include "checks/level1/qhash-namespace.h"
#include "checks/level2/base-class-event.h"
#include "checks/level2/container-inside-loop.h"
#include "checks/level2/ctor-missing-parent-argument.h"
#include "checks/level2/copyable-polymorphic.h"
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
#include "checks/level3/bogus-dynamic-cast.h"
#include "checks/level3/thread-with-slots.h"

void CheckManager::registerChecks()
{
    registerCheck("inefficient-qlist", HiddenCheckLevel, [](ClazyContext *context){ return new InefficientQList("inefficient-qlist", context); });
    registerCheck("isempty-vs-count", HiddenCheckLevel, [](ClazyContext *context){ return new IsEmptyVSCount("isempty-vs-count", context); });
    registerCheck("qt4-qstring-from-array", HiddenCheckLevel, [](ClazyContext *context){ return new Qt4QStringFromArray("qt4-qstring-from-array", context); });
    registerFixIt(1, "fix-qt4-qstring-from-array", "qt4-qstring-from-array");
    registerCheck("tr-non-literal", HiddenCheckLevel, [](ClazyContext *context){ return new TrNonLiteral("tr-non-literal", context); });
    registerCheck("connect-non-signal", CheckLevel0, [](ClazyContext *context){ return new ConnectNonSignal("connect-non-signal", context); }, RegisteredCheck::Option_Qt4Incompatible);
    registerCheck("lambda-in-connect", CheckLevel0, [](ClazyContext *context){ return new LambdaInConnect("lambda-in-connect", context); });
    registerCheck("lambda-unique-connection", CheckLevel0, [](ClazyContext *context){ return new LambdaUniqueConnection("lambda-unique-connection", context); });
    registerCheck("qdatetime-utc", CheckLevel0, [](ClazyContext *context){ return new QDateTimeUtc("qdatetime-utc", context); });
    registerFixIt(1, "fix-qdatetime-utc", "qdatetime-utc");
    registerCheck("qgetenv", CheckLevel0, [](ClazyContext *context){ return new QGetEnv("qgetenv", context); }, RegisteredCheck::Option_Qt4Incompatible);
    registerFixIt(1, "fix-qgetenv", "qgetenv");
    registerCheck("qstring-insensitive-allocation", CheckLevel0, [](ClazyContext *context){ return new QStringInsensitiveAllocation("qstring-insensitive-allocation", context); });
    registerCheck("qvariant-template-instantiation", CheckLevel0, [](ClazyContext *context){ return new QVariantTemplateInstantiation("qvariant-template-instantiation", context); });
    registerCheck("unused-non-trivial-variable", CheckLevel0, [](ClazyContext *context){ return new UnusedNonTrivialVariable("unused-non-trivial-variable", context); });
    registerCheck("connect-not-normalized", CheckLevel0, [](ClazyContext *context){ return new ConnectNotNormalized("connect-not-normalized", context); });
    registerCheck("mutable-container-key", CheckLevel0, [](ClazyContext *context){ return new MutableContainerKey("mutable-container-key", context); });
    registerCheck("qenums", CheckLevel0, [](ClazyContext *context){ return new QEnums("qenums", context); }, RegisteredCheck::Option_Qt4Incompatible);
    registerCheck("qmap-with-pointer-key", CheckLevel0, [](ClazyContext *context){ return new QMapWithPointerKey("qmap-with-pointer-key", context); });
    registerCheck("qstring-ref", CheckLevel0, [](ClazyContext *context){ return new StringRefCandidates("qstring-ref", context); });
    registerFixIt(1, "fix-missing-qstringref", "qstring-ref");
    registerCheck("strict-iterators", CheckLevel0, [](ClazyContext *context){ return new StrictIterators("strict-iterators", context); });
    registerCheck("writing-to-temporary", CheckLevel0, [](ClazyContext *context){ return new WritingToTemporary("writing-to-temporary", context); });
    registerCheck("container-anti-pattern", CheckLevel0, [](ClazyContext *context){ return new ContainerAntiPattern("container-anti-pattern", context); });
    registerCheck("qcolor-from-literal", CheckLevel0, [](ClazyContext *context){ return new QColorFromLiteral("qcolor-from-literal", context); });
    registerCheck("qfileinfo-exists", CheckLevel0, [](ClazyContext *context){ return new QFileInfoExists("qfileinfo-exists", context); });
    registerCheck("qstring-arg", CheckLevel0, [](ClazyContext *context){ return new QStringArg("qstring-arg", context); });
    registerCheck("qt-macros", CheckLevel0, [](ClazyContext *context){ return new QtMacros("qt-macros", context); });
    registerCheck("temporary-iterator", CheckLevel0, [](ClazyContext *context){ return new TemporaryIterator("temporary-iterator", context); });
    registerCheck("wrong-qglobalstatic", CheckLevel0, [](ClazyContext *context){ return new WrongQGlobalStatic("wrong-qglobalstatic", context); });
    registerCheck("auto-unexpected-qstringbuilder", CheckLevel1, [](ClazyContext *context){ return new AutoUnexpectedQStringBuilder("auto-unexpected-qstringbuilder", context); });
    registerFixIt(1, "fix-auto-unexpected-qstringbuilder", "auto-unexpected-qstringbuilder");
    registerCheck("connect-3arg-lambda", CheckLevel1, [](ClazyContext *context){ return new Connect3ArgLambda("connect-3arg-lambda", context); });
    registerCheck("const-signal-or-slot", CheckLevel1, [](ClazyContext *context){ return new ConstSignalOrSlot("const-signal-or-slot", context); });
    registerCheck("detaching-temporary", CheckLevel1, [](ClazyContext *context){ return new DetachingTemporary("detaching-temporary", context); });
    registerCheck("foreach", CheckLevel1, [](ClazyContext *context){ return new Foreach("foreach", context); });
    registerCheck("incorrect-emit", CheckLevel1, [](ClazyContext *context){ return new IncorrectEmit("incorrect-emit", context); });
    registerCheck("inefficient-qlist-soft", CheckLevel1, [](ClazyContext *context){ return new InefficientQListSoft("inefficient-qlist-soft", context); });
    registerCheck("install-event-filter", CheckLevel1, [](ClazyContext *context){ return new InstallEventFilter("install-event-filter", context); });
    registerCheck("non-pod-global-static", CheckLevel1, [](ClazyContext *context){ return new NonPodGlobalStatic("non-pod-global-static", context); });
    registerCheck("post-event", CheckLevel1, [](ClazyContext *context){ return new PostEvent("post-event", context); });
    registerCheck("qdeleteall", CheckLevel1, [](ClazyContext *context){ return new QDeleteAll("qdeleteall", context); });
    registerCheck("qlatin1string-non-ascii", CheckLevel1, [](ClazyContext *context){ return new QLatin1StringNonAscii("qlatin1string-non-ascii", context); });
    registerCheck("qproperty-without-notify", CheckLevel1, [](ClazyContext *context){ return new QPropertyWithoutNotify("qproperty-without-notify", context); });
    registerCheck("qstring-left", CheckLevel1, [](ClazyContext *context){ return new QStringLeft("qstring-left", context); });
    registerCheck("range-loop", CheckLevel1, [](ClazyContext *context){ return new RangeLoop("range-loop", context); });
    registerCheck("returning-data-from-temporary", CheckLevel1, [](ClazyContext *context){ return new ReturningDataFromTemporary("returning-data-from-temporary", context); });
    registerCheck("rule-of-two-soft", CheckLevel1, [](ClazyContext *context){ return new RuleOfTwoSoft("rule-of-two-soft", context); });
    registerCheck("child-event-qobject-cast", CheckLevel1, [](ClazyContext *context){ return new ChildEventQObjectCast("child-event-qobject-cast", context); });
    registerCheck("virtual-signal", CheckLevel1, [](ClazyContext *context){ return new VirtualSignal("virtual-signal", context); });
    registerCheck("overridden-signal", CheckLevel1, [](ClazyContext *context){ return new OverriddenSignal("overridden-signal", context); });
    registerCheck("qhash-namespace", CheckLevel1, [](ClazyContext *context){ return new QHashNamespace("qhash-namespace", context); });
    registerCheck("base-class-event", CheckLevel2, [](ClazyContext *context){ return new BaseClassEvent("base-class-event", context); });
    registerCheck("container-inside-loop", CheckLevel2, [](ClazyContext *context){ return new ContainerInsideLoop("container-inside-loop", context); });
    registerCheck("ctor-missing-parent-argument", CheckLevel2, [](ClazyContext *context){ return new CtorMissingParentArgument("ctor-missing-parent-argument", context); });
    registerCheck("copyable-polymorphic", CheckLevel2, [](ClazyContext *context){ return new CopyablePolymorphic("copyable-polymorphic", context); });
    registerCheck("function-args-by-ref", CheckLevel2, [](ClazyContext *context){ return new FunctionArgsByRef("function-args-by-ref", context); });
    registerCheck("function-args-by-value", CheckLevel2, [](ClazyContext *context){ return new FunctionArgsByValue("function-args-by-value", context); });
    registerCheck("global-const-char-pointer", CheckLevel2, [](ClazyContext *context){ return new GlobalConstCharPointer("global-const-char-pointer", context); });
    registerCheck("implicit-casts", CheckLevel2, [](ClazyContext *context){ return new ImplicitCasts("implicit-casts", context); });
    registerCheck("missing-qobject-macro", CheckLevel2, [](ClazyContext *context){ return new MissingQObjectMacro("missing-qobject-macro", context); });
    registerCheck("missing-typeinfo", CheckLevel2, [](ClazyContext *context){ return new MissingTypeInfo("missing-typeinfo", context); });
    registerCheck("old-style-connect", CheckLevel2, [](ClazyContext *context){ return new OldStyleConnect("old-style-connect", context); }, RegisteredCheck::Option_Qt4Incompatible);
    registerFixIt(1, "fix-old-style-connect", "old-style-connect");
    registerCheck("qstring-allocations", CheckLevel2, [](ClazyContext *context){ return new QStringAllocations("qstring-allocations", context); }, RegisteredCheck::Option_Qt4Incompatible);
    registerFixIt(1, "fix-qlatin1string-allocations", "qstring-allocations");
    registerFixIt(2, "fix-fromLatin1_fromUtf8-allocations", "qstring-allocations");
    registerFixIt(4, "fix-fromCharPtrAllocations", "qstring-allocations");
    registerCheck("reserve-candidates", CheckLevel2, [](ClazyContext *context){ return new ReserveCandidates("reserve-candidates", context); });
    registerCheck("returning-void-expression", CheckLevel2, [](ClazyContext *context){ return new ReturningVoidExpression("returning-void-expression", context); });
    registerCheck("rule-of-three", CheckLevel2, [](ClazyContext *context){ return new RuleOfThree("rule-of-three", context); });
    registerCheck("virtual-call-ctor", CheckLevel2, [](ClazyContext *context){ return new VirtualCallCtor("virtual-call-ctor", context); });
    registerCheck("assert-with-side-effects", CheckLevel3, [](ClazyContext *context){ return new AssertWithSideEffects("assert-with-side-effects", context); });
    registerCheck("detaching-member", CheckLevel3, [](ClazyContext *context){ return new DetachingMember("detaching-member", context); });
    registerCheck("bogus-dynamic-cast", CheckLevel3, [](ClazyContext *context){ return new BogusDynamicCast("bogus-dynamic-cast", context); });
    registerCheck("thread-with-slots", CheckLevel3, [](ClazyContext *context){ return new ThreadWithSlots("thread-with-slots", context); });
}

