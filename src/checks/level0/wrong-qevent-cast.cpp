/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "wrong-qevent-cast.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "StringUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <unordered_map>
#include <utility>
#include <vector>

using namespace clang;
using namespace std;

typedef vector<StringRef> ClassNameList;

enum QtUnregularlyNamedEventTypes {
    DragEnter = 60,
    DragLeave = 62,
    OrientationChange = 208,
    ActionAdded = 114,
    ActionRemoved = 115,
    ActionChanged = 99,
    ChildAdded = 68,
    ChildRemoved = 71,
    ChildPolished = 69,
    MouseButtonPress = 2,
    MouseButtonRelease = 3,
    MouseButtonDblClick = 4,
    MouseMove = 5,
    NonClientAreaMouseMove = 173,
    NonClientAreaMouseButtonPress = 174,
    NonClientAreaMouseButtonRelease = 175,
    NonClientAreaMouseButtonDblClick = 176,
    FocusIn = 8,
    FocusOut = 9,
    FocusAboutToChange = 23,
    Gesture = 198,
    GestureOverride = 202,
    HoverEnter = 127,
    HoverLeave = 128,
    HoverMove = 129,
    TabletEnterProximity = 171,
    TabletLeaveProximity = 172,
    TabletPress = 92,
    TabletMove = 87,
    TabletRelease = 93,
    ToolTip = 110,
    Wheel = 31,
    KeyPress = 6,
    KeyRelease = 7,
    ShortcutOverride = 51,
    DragMove = 61,
    GraphicsSceneMouseMove = 155,
    GraphicsSceneMousePress = 156,
    GraphicsSceneMouseRelease = 157,
    GraphicsSceneMouseDoubleClick = 158,
    GraphicsSceneContextMenu = 159,
    GraphicsSceneHoverEnter = 160,
    GraphicsSceneHoverMove = 161,
    GraphicsSceneHoverLeave = 162,
    GraphicsSceneHelp = 163,
    GraphicsSceneDragEnter = 164,
    GraphicsSceneDragMove = 165,
    GraphicsSceneDragLeave = 166,
    GraphicsSceneDrop = 167,
    GraphicsSceneWheel = 168,
    GraphicsSceneResize = 181,
    TouchBegin = 194,
    TouchEnd = 196,
    TouchCancel = 209,
    TouchUpdate = 195,
    NativeGesture = 197,
    MetaCall = 43,
    WhatsThis = 111,
    ContextMenu = 82,
    QueryWhatsThis = 123
                     // StatusTip = 112 not irregular, but qtbase casts it to QHelpEvent for some reason, needs investigation
};


WrongQEventCast::WrongQEventCast(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{


}

static bool eventTypeMatchesClass(QtUnregularlyNamedEventTypes eventType, string eventTypeStr, StringRef className)
{
    // In the simplest case, the class is "Q" + eventType + "Event"
    string expectedClassName = string("Q") + eventTypeStr + string("Event");
    if (expectedClassName == className)
        return true;

    // Otherwise it's unregular and we need a map:

    static unordered_map<QtUnregularlyNamedEventTypes, ClassNameList, std::hash<int>> map = {
        { ActionAdded, {"QActionEvent" } },
        { ActionRemoved, {"QActionEvent" } },
        { ActionChanged, {"QActionEvent" } },
        { ChildAdded, {"QChildEvent" } },
        { ChildRemoved, {"QChildEvent" } },
        { ChildPolished, {"QChildEvent" } },
        { MetaCall, {"QDBusSpyCallEvent", "QDBusCallDeliveryEvent"} },
        { DragEnter, {"QDragEnterEvent", "QDragMoveEvent", "QDropEvent"  } },
        { DragLeave, {"QDragLeaveEvent", "QDragMoveEvent", "QDropEvent" } },
        { DragMove, {"QDragMoveEvent", "QDropEvent" } },
        { FocusIn, {"QFocusEvent" } },
        { FocusOut, {"QFocusEvent" } },
        { FocusAboutToChange, {"QFocusEvent" } },
        { Gesture, {"QGestureEvent" } },
        { GestureOverride, {"QGestureEvent" } },
        { GraphicsSceneContextMenu, {"QGraphicsSceneEvent" } },
        { GraphicsSceneHoverEnter, { "QGraphicsSceneHoverEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneHoverMove, {"QGraphicsSceneHoverEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneHoverLeave, {"QGraphicsSceneHoverEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneHelp, { "QGraphicsSceneEvent" } },
        { GraphicsSceneDragEnter, {"QGraphicsSceneDragDropEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneDragMove, {"QGraphicsSceneDragDropEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneDragLeave, {"QGraphicsSceneDragDropEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneDrop, {"QGraphicsSceneDragDropEvent", "QGraphicsSceneEvent" } },
        { GraphicsSceneWheel, {"QGraphicsSceneEvent" } },
        { GraphicsSceneResize, {"QGraphicsSceneEvent" } },
        { GraphicsSceneMouseMove, {"QGraphicsSceneMouseEvent" } },
        { GraphicsSceneMousePress, {"QGraphicsSceneMouseEvent" } },
        { GraphicsSceneMouseRelease, {"QGraphicsSceneMouseEvent" } },
        { GraphicsSceneMouseDoubleClick, {"QGraphicsSceneMouseEvent" } },
        //{ StatusTip, {"QStatusTipEvent" } },
        { ToolTip, {"QHelpEvent" } },
        { WhatsThis, {"QHelpEvent" } },
        { QueryWhatsThis, {"QHelpEvent" } },
        { HoverEnter, {"QHoverEvent", "QInputEvent" } },
        { HoverLeave, {"QHoverEvent", "QInputEvent" } },
        { HoverMove, {"QHoverEvent", "QInputEvent" } },
        { KeyPress, {"QKeyEvent", "QInputEvent" } },
        { KeyRelease, {"QKeyEvent", "QInputEvent" } },
        { ShortcutOverride, {"QKeyEvent", "QInputEvent" } },
        { MouseButtonPress, {"QMouseEvent" } },
        { MouseButtonRelease, {"QMouseEvent" } },
        { MouseButtonDblClick, {"QMouseEvent" } },
        { MouseMove, {"QMouseEvent" } },
        { NonClientAreaMouseMove, {"QMouseEvent" } },
        { NonClientAreaMouseButtonPress, {"QMouseEvent" } },
        { NonClientAreaMouseButtonRelease, {"QMouseEvent" } },
        { NonClientAreaMouseButtonRelease, {"QMouseEvent" } },
        { NonClientAreaMouseButtonDblClick, {"QMouseEvent" } },
        { NativeGesture, { "QInputEvent" } },
        { OrientationChange, {"QScreenOrientationChangeEvent" } },
        { TabletEnterProximity, {"QTabletEvent", "QInputEvent" } },
        { TabletLeaveProximity, {"QTabletEvent", "QInputEvent" } },
        { TabletPress, {"QTabletEvent", "QInputEvent" } },
        { TabletMove, {"QTabletEvent", "QInputEvent" } },
        { TabletRelease, {"QTabletEvent", "QInputEvent" } },
        { TouchBegin, {"QTouchEvent", "QInputEvent" } },
        { TouchCancel, {"QTouchEvent", "QInputEvent" } },
        { TouchEnd, {"QTouchEvent", "QInputEvent" } },
        { TouchUpdate, {"QTouchEvent", "QInputEvent" } },
        { Wheel, {"QInputEvent" } },
        { ContextMenu, {"QInputEvent" } }
    };

    auto it = map.find(eventType);
    if (it == map.cend())
        return false;

    const ClassNameList &classes = it->second;
    const bool found = clazy::find(classes, className) != classes.cend();

    return found;
}


// TODO: Use iterators
CaseStmt* getCaseStatement(clang::ParentMap *pmap, Stmt *stmt, DeclRefExpr *event)
{
    Stmt *s = pmap->getParent(stmt);

    while (s) {

        if (auto ifStmt = dyn_cast<IfStmt>(s)) {
            // if there's we're inside an if statement then skip, to avoid false-positives
            auto declRef = clazy::getFirstChildOfType2<DeclRefExpr>(ifStmt->getCond());
            if (declRef && declRef->getDecl() == event->getDecl())
                return nullptr;
        }


        if (auto caseStmt = dyn_cast<CaseStmt>(s)) {
            auto switchStmt = clazy::getSwitchFromCase(pmap, caseStmt);
            if (switchStmt) {
                auto declRef = clazy::getFirstChildOfType2<DeclRefExpr>(switchStmt->getCond());
                // Does this switch refer to the same QEvent ?
                if (declRef && declRef->getDecl() == event->getDecl())
                    return caseStmt;
            }
        }

        s = pmap->getParent(s);
    }

    return nullptr;
}

void WrongQEventCast::VisitStmt(clang::Stmt *stmt)
{
    auto cast = dyn_cast<CXXStaticCastExpr>(stmt);
    if (!cast)
        return;

    Expr *e = cast->getSubExpr();

    QualType t = e ? e->getType() : QualType();
    QualType pointeeType = t.isNull() ? QualType() : clazy::pointeeQualType(t);
    CXXRecordDecl *rec = pointeeType.isNull() ? nullptr : pointeeType->getAsCXXRecordDecl();

    if (!rec || clazy::name(rec) != "QEvent")
        return;

    CXXRecordDecl *castTo = Utils::namedCastOuterDecl(cast);
    if (!castTo)
        return;

    auto declref = clazy::getFirstChildOfType2<DeclRefExpr>(cast->getSubExpr());
    if (!declref)
        return;

    auto caseStmt = getCaseStatement(m_context->parentMap, stmt, declref);
    if (!caseStmt)
        return;

    auto caseValue = clazy::getFirstChildOfType2<DeclRefExpr>(caseStmt->getLHS());
    if (!caseValue)
        return;


    auto enumeratorDecl = dyn_cast<EnumConstantDecl>(caseValue->getDecl());
    if (!enumeratorDecl)
        return;

    auto enumeratorVal = static_cast<QtUnregularlyNamedEventTypes>(enumeratorDecl->getInitVal().getExtValue());

    string eventTypeStr = enumeratorDecl->getNameAsString();
    StringRef castToName = clazy::name(castTo);

    if (eventTypeMatchesClass(enumeratorVal, eventTypeStr, castToName))
        return;

    emitWarning(stmt, string("Cast from a QEvent::") + eventTypeStr + " event to " + string(castToName) + " looks suspicious.");
}
