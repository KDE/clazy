/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

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

#include "qt6-header-fixes.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "StringUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/Lex/Lexer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;

static bool newOldHeaderFileMatch(string FileNameOld, string &FileNameNew)
{
    static unordered_map<string, string> map = {
        { "ActiveQt/QAxAggregated", "QtAxServer/QAxAggregated" },
        { "ActiveQt/QAxBase", "QtAxContainer/QAxBase" },
        { "ActiveQt/QAxBindable", "QtAxServer/QAxBindable" },
        { "ActiveQt/QAxFactory", "QtAxServer/QAxFactory" },
        { "ActiveQt/QAxObject", "QtAxContainer/QAxObject" },
        { "ActiveQt/QAxScript", "QtAxContainer/QAxScript" },
        { "ActiveQt/QAxScriptEngine", "QtAxContainer/QAxScriptEngine" },
        { "ActiveQt/QAxScriptManager", "QtAxContainer/QAxScriptManager" },
        { "ActiveQt/QAxSelect", "QtAxContainer/QAxSelect" },
        { "ActiveQt/QAxWidget", "QtAxContainer/QAxWidget" },
        { "ActiveQt/qaxaggregated.h", "QtAxServer/qaxaggregated.h" },
        { "ActiveQt/qaxbase.h", "QtAxContainer/qaxbase.h" },
        { "ActiveQt/qaxbindable.h", "QtAxServer/qaxbindable.h" },
        { "ActiveQt/qaxfactory.h", "QtAxServer/qaxfactory.h" },
        { "ActiveQt/qaxobject.h", "QtAxContainer/qaxobject.h" },
        { "ActiveQt/qaxscript.h", "QtAxContainer/qaxscript.h" },
        { "ActiveQt/qaxselect.h", "QtAxContainer/qaxselect.h" },
        { "ActiveQt/qaxwidget.h", "QtAxContainer/qaxwidget.h" },
        { "Qt3DAnimation/QAbstractAnimation", "QtCore/QAbstractAnimation" },
        { "Qt3DAnimation/QAnimationGroup", "QtCore/QAnimationGroup" },
        { "Qt3DAnimation/qabstractanimation.h", "QtCore/qabstractanimation.h" },
        { "Qt3DAnimation/qanimationgroup.h", "QtCore/qanimationgroup.h" },
        { "Qt3DCore/QTransform", "QtGui/QTransform" },
        { "Qt3DCore/qtransform.h", "QtGui/qtransform.h" },
        { "Qt3DInput/QAction", "QtGui/QAction" },
        { "Qt3DInput/QKeyEvent", "QtGui/QKeyEvent" },
        { "Qt3DInput/QMouseEvent", "QtGui/QMouseEvent" },
        { "Qt3DInput/QWheelEvent", "QtGui/QWheelEvent" },
        { "Qt3DInput/qaction.h", "QtGui/qaction.h" },
        { "Qt3DRender/FunctorType", "Qt3DCore/FunctorType" },
        { "Qt3DRender/QAbstractFunctor", "Qt3DCore/QAbstractFunctor" },
        { "Qt3DRender/QAttribute", "Qt3DCore/QAttribute" },
        { "Qt3DRender/QBuffer", "QtCore/QBuffer" },
        { "Qt3DRender/QBufferPtr", "Qt3DCore/QBufferPtr" },
        { "Qt3DRender/QCamera", "QtMultimedia/QCamera" },
        { "Qt3DRender/QGeometry", "Qt3DCore/QGeometry" },
        { "Qt3DRender/QGeometryFactoryPtr", "Qt3DCore/QGeometryFactoryPtr" },
        { "Qt3DRender/QProximityFilter", "QtSensors/QProximityFilter" },
        { "Qt3DRender/qabstractfunctor.h", "Qt3DCore/qabstractfunctor.h" },
        { "Qt3DRender/qattribute.h", "Qt3DCore/qattribute.h" },
        { "Qt3DRender/qbuffer.h", "QtCore/qbuffer.h" },
        { "Qt3DRender/qcamera.h", "QtMultimedia/qcamera.h" },
        { "Qt3DRender/qgeometry.h", "Qt3DCore/qgeometry.h" },
        { "QtCore/QAbstractState", "QtStateMachine/QAbstractState" },
        { "QtCore/QAbstractTransition", "QtStateMachine/QAbstractTransition" },
        { "QtCore/QEventTransition", "QtStateMachine/QEventTransition" },
        { "QtCore/QFinalState", "QtStateMachine/QFinalState" },
        { "QtCore/QHistoryState", "QtStateMachine/QHistoryState" },
        { "QtCore/QSignalTransition", "QtStateMachine/QSignalTransition" },
        { "QtCore/QState", "QtStateMachine/QState" },
        { "QtCore/QStateMachine", "QtStateMachine/QStateMachine" },
        { "QtCore/qabstractstate.h", "QtStateMachine/qabstractstate.h" },
        { "QtCore/qabstracttransition.h", "QtStateMachine/qabstracttransition.h" },
        { "QtCore/qeventtransition.h", "QtStateMachine/qeventtransition.h" },
        { "QtCore/qfinalstate.h", "QtStateMachine/qfinalstate.h" },
        { "QtCore/qhistorystate.h", "QtStateMachine/qhistorystate.h" },
        { "QtCore/qsignaltransition.h", "QtStateMachine/qsignaltransition.h" },
        { "QtCore/qstate.h", "QtStateMachine/qstate.h" },
        { "QtCore/qstatemachine.h", "QtStateMachine/qstatemachine.h" },
        { "QtDesigner/QDesignerCustomWidgetCollectionInterface", "QtUiPlugin/QDesignerCustomWidgetCollectionInterface" },
        { "QtDesigner/QDesignerCustomWidgetInterface", "QtUiPlugin/QDesignerCustomWidgetInterface" },
        { "QtDesigner/QDesignerExportWidget", "QtUiPlugin/QDesignerExportWidget" },
        { "QtDesigner/customwidget.h", "QtUiPlugin/customwidget.h" },
        { "QtDesigner/qdesignerexportwidget.h", "QtUiPlugin/qdesignerexportwidget.h" },
        { "QtGui/QOpenGLBuffer", "QtOpenGL/QOpenGLBuffer" },
        { "QtGui/QOpenGLDebugLogger", "QtOpenGL/QOpenGLDebugLogger" },
        { "QtGui/QOpenGLDebugMessage", "QtOpenGL/QOpenGLDebugMessage" },
        { "QtGui/QOpenGLFramebufferObject", "QtOpenGL/QOpenGLFramebufferObject" },
        { "QtGui/QOpenGLFramebufferObjectFormat", "QtOpenGL/QOpenGLFramebufferObjectFormat" },
        { "QtGui/QOpenGLFunctions_1_0", "QtOpenGL/QOpenGLFunctions_1_0" },
        { "QtGui/QOpenGLFunctions_1_1", "QtOpenGL/QOpenGLFunctions_1_1" },
        { "QtGui/QOpenGLFunctions_1_2", "QtOpenGL/QOpenGLFunctions_1_2" },
        { "QtGui/QOpenGLFunctions_1_3", "QtOpenGL/QOpenGLFunctions_1_3" },
        { "QtGui/QOpenGLFunctions_1_4", "QtOpenGL/QOpenGLFunctions_1_4" },
        { "QtGui/QOpenGLFunctions_1_5", "QtOpenGL/QOpenGLFunctions_1_5" },
        { "QtGui/QOpenGLFunctions_2_0", "QtOpenGL/QOpenGLFunctions_2_0" },
        { "QtGui/QOpenGLFunctions_2_1", "QtOpenGL/QOpenGLFunctions_2_1" },
        { "QtGui/QOpenGLFunctions_3_0", "QtOpenGL/QOpenGLFunctions_3_0" },
        { "QtGui/QOpenGLFunctions_3_1", "QtOpenGL/QOpenGLFunctions_3_1" },
        { "QtGui/QOpenGLFunctions_3_2_Compatibility", "QtOpenGL/QOpenGLFunctions_3_2_Compatibility" },
        { "QtGui/QOpenGLFunctions_3_2_Core", "QtOpenGL/QOpenGLFunctions_3_2_Core" },
        { "QtGui/QOpenGLFunctions_3_3_Compatibility", "QtOpenGL/QOpenGLFunctions_3_3_Compatibility" },
        { "QtGui/QOpenGLFunctions_3_3_Core", "QtOpenGL/QOpenGLFunctions_3_3_Core" },
        { "QtGui/QOpenGLFunctions_4_0_Compatibility", "QtOpenGL/QOpenGLFunctions_4_0_Compatibility" },
        { "QtGui/QOpenGLFunctions_4_0_Core", "QtOpenGL/QOpenGLFunctions_4_0_Core" },
        { "QtGui/QOpenGLFunctions_4_1_Compatibility", "QtOpenGL/QOpenGLFunctions_4_1_Compatibility" },
        { "QtGui/QOpenGLFunctions_4_1_Core", "QtOpenGL/QOpenGLFunctions_4_1_Core" },
        { "QtGui/QOpenGLFunctions_4_2_Compatibility", "QtOpenGL/QOpenGLFunctions_4_2_Compatibility" },
        { "QtGui/QOpenGLFunctions_4_2_Core", "QtOpenGL/QOpenGLFunctions_4_2_Core" },
        { "QtGui/QOpenGLFunctions_4_3_Compatibility", "QtOpenGL/QOpenGLFunctions_4_3_Compatibility" },
        { "QtGui/QOpenGLFunctions_4_3_Core", "QtOpenGL/QOpenGLFunctions_4_3_Core" },
        { "QtGui/QOpenGLFunctions_4_4_Compatibility", "QtOpenGL/QOpenGLFunctions_4_4_Compatibility" },
        { "QtGui/QOpenGLFunctions_4_4_Core", "QtOpenGL/QOpenGLFunctions_4_4_Core" },
        { "QtGui/QOpenGLFunctions_4_5_Compatibility", "QtOpenGL/QOpenGLFunctions_4_5_Compatibility" },
        { "QtGui/QOpenGLFunctions_4_5_Core", "QtOpenGL/QOpenGLFunctions_4_5_Core" },
        { "QtGui/QOpenGLFunctions_ES2", "QtOpenGL/QOpenGLFunctions_ES2" },
        { "QtGui/QOpenGLPaintDevice", "QtOpenGL/QOpenGLPaintDevice" },
        { "QtGui/QOpenGLPixelTransferOptions", "QtOpenGL/QOpenGLPixelTransferOptions" },
        { "QtGui/QOpenGLShader", "QtOpenGL/QOpenGLShader" },
        { "QtGui/QOpenGLShaderProgram", "QtOpenGL/QOpenGLShaderProgram" },
        { "QtGui/QOpenGLTexture", "QtOpenGL/QOpenGLTexture" },
        { "QtGui/QOpenGLTextureBlitter", "QtOpenGL/QOpenGLTextureBlitter" },
        { "QtGui/QOpenGLTimeMonitor", "QtOpenGL/QOpenGLTimeMonitor" },
        { "QtGui/QOpenGLTimerQuery", "QtOpenGL/QOpenGLTimerQuery" },
        { "QtGui/QOpenGLVersionFunctions", "QtOpenGL/QOpenGLVersionFunctions" },
        { "QtGui/QOpenGLVersionProfile", "QtOpenGL/QOpenGLVersionProfile" },
        { "QtGui/QOpenGLVertexArrayObject", "QtOpenGL/QOpenGLVertexArrayObject" },
        { "QtGui/QOpenGLWindow", "QtOpenGL/QOpenGLWindow" },
        { "QtGui/qopenglbuffer.h", "QtOpenGL/qopenglbuffer.h" },
        { "QtGui/qopengldebug.h", "QtOpenGL/qopengldebug.h" },
        { "QtGui/qopenglframebufferobject.h", "QtOpenGL/qopenglframebufferobject.h" },
        { "QtGui/qopenglfunctions_1_0.h", "QtOpenGL/qopenglfunctions_1_0.h" },
        { "QtGui/qopenglfunctions_1_1.h", "QtOpenGL/qopenglfunctions_1_1.h" },
        { "QtGui/qopenglfunctions_1_2.h", "QtOpenGL/qopenglfunctions_1_2.h" },
        { "QtGui/qopenglfunctions_1_3.h", "QtOpenGL/qopenglfunctions_1_3.h" },
        { "QtGui/qopenglfunctions_1_4.h", "QtOpenGL/qopenglfunctions_1_4.h" },
        { "QtGui/qopenglfunctions_1_5.h", "QtOpenGL/qopenglfunctions_1_5.h" },
        { "QtGui/qopenglfunctions_2_0.h", "QtOpenGL/qopenglfunctions_2_0.h" },
        { "QtGui/qopenglfunctions_2_1.h", "QtOpenGL/qopenglfunctions_2_1.h" },
        { "QtGui/qopenglfunctions_3_0.h", "QtOpenGL/qopenglfunctions_3_0.h" },
        { "QtGui/qopenglfunctions_3_1.h", "QtOpenGL/qopenglfunctions_3_1.h" },
        { "QtGui/qopenglfunctions_3_2_compatibility.h", "QtOpenGL/qopenglfunctions_3_2_compatibility.h" },
        { "QtGui/qopenglfunctions_3_2_core.h", "QtOpenGL/qopenglfunctions_3_2_core.h" },
        { "QtGui/qopenglfunctions_3_3_compatibility.h", "QtOpenGL/qopenglfunctions_3_3_compatibility.h" },
        { "QtGui/qopenglfunctions_3_3_core.h", "QtOpenGL/qopenglfunctions_3_3_core.h" },
        { "QtGui/qopenglfunctions_4_0_compatibility.h", "QtOpenGL/qopenglfunctions_4_0_compatibility.h" },
        { "QtGui/qopenglfunctions_4_0_core.h", "QtOpenGL/qopenglfunctions_4_0_core.h" },
        { "QtGui/qopenglfunctions_4_1_compatibility.h", "QtOpenGL/qopenglfunctions_4_1_compatibility.h" },
        { "QtGui/qopenglfunctions_4_1_core.h", "QtOpenGL/qopenglfunctions_4_1_core.h" },
        { "QtGui/qopenglfunctions_4_2_compatibility.h", "QtOpenGL/qopenglfunctions_4_2_compatibility.h" },
        { "QtGui/qopenglfunctions_4_2_core.h", "QtOpenGL/qopenglfunctions_4_2_core.h" },
        { "QtGui/qopenglfunctions_4_3_compatibility.h", "QtOpenGL/qopenglfunctions_4_3_compatibility.h" },
        { "QtGui/qopenglfunctions_4_3_core.h", "QtOpenGL/qopenglfunctions_4_3_core.h" },
        { "QtGui/qopenglfunctions_4_4_compatibility.h", "QtOpenGL/qopenglfunctions_4_4_compatibility.h" },
        { "QtGui/qopenglfunctions_4_4_core.h", "QtOpenGL/qopenglfunctions_4_4_core.h" },
        { "QtGui/qopenglfunctions_4_5_compatibility.h", "QtOpenGL/qopenglfunctions_4_5_compatibility.h" },
        { "QtGui/qopenglfunctions_4_5_core.h", "QtOpenGL/qopenglfunctions_4_5_core.h" },
        { "QtGui/qopenglfunctions_es2.h", "QtOpenGL/qopenglfunctions_es2.h" },
        { "QtGui/qopenglpaintdevice.h", "QtOpenGL/qopenglpaintdevice.h" },
        { "QtGui/qopenglpixeltransferoptions.h", "QtOpenGL/qopenglpixeltransferoptions.h" },
        { "QtGui/qopenglshaderprogram.h", "QtOpenGL/qopenglshaderprogram.h" },
        { "QtGui/qopengltexture.h", "QtOpenGL/qopengltexture.h" },
        { "QtGui/qopengltextureblitter.h", "QtOpenGL/qopengltextureblitter.h" },
        { "QtGui/qopengltimerquery.h", "QtOpenGL/qopengltimerquery.h" },
        { "QtGui/qopenglversionfunctions.h", "QtOpenGL/qopenglversionfunctions.h" },
        { "QtGui/qopenglvertexarrayobject.h", "QtOpenGL/qopenglvertexarrayobject.h" },
        { "QtGui/qopenglwindow.h", "QtOpenGL/qopenglwindow.h" },
        { "QtSvg/QGraphicsSvgItem", "QtSvgWidgets/QGraphicsSvgItem" },
        { "QtSvg/QSvgWidget", "QtSvgWidgets/QSvgWidget" },
        { "QtSvg/qgraphicssvgitem.h", "QtSvgWidgets/qgraphicssvgitem.h" },
        { "QtSvg/qsvgwidget.h", "QtSvgWidgets/qsvgwidget.h" },
        { "QtWidgets/QAction", "QtGui/QAction" },
        { "QtWidgets/QActionGroup", "QtGui/QActionGroup" },
        { "QtWidgets/QFileSystemModel", "QtGui/QFileSystemModel" },
        { "QtWidgets/QKeyEventTransition", "QtStateMachine/QKeyEventTransition" },
        { "QtWidgets/QMouseEventTransition", "QtStateMachine/QMouseEventTransition" },
        { "QtWidgets/QOpenGLWidget", "QtOpenGLWidgets/QOpenGLWidget" },
        { "QtWidgets/QShortcut", "QtGui/QShortcut" },
        { "QtWidgets/QUndoCommand", "QtGui/QUndoCommand" },
        { "QtWidgets/QUndoGroup", "QtGui/QUndoGroup" },
        { "QtWidgets/QUndoStack", "QtGui/QUndoStack" },
        { "QtWidgets/qaction.h", "QtGui/qaction.h" },
        { "QtWidgets/qactiongroup.h", "QtGui/qactiongroup.h" },
        { "QtWidgets/qfilesystemmodel.h", "QtGui/qfilesystemmodel.h" },
        { "QtWidgets/qkeyeventtransition.h", "QtStateMachine/qkeyeventtransition.h" },
        { "QtWidgets/qmouseeventtransition.h", "QtStateMachine/qmouseeventtransition.h" },
        { "QtWidgets/qopenglwidget.h", "QtOpenGLWidgets/qopenglwidget.h" },
        { "QtWidgets/qshortcut.h", "QtGui/qshortcut.h" },
        { "QtWidgets/qundogroup.h", "QtGui/qundogroup.h" },
        { "QtWidgets/qundostack.h", "QtGui/qundostack.h" }
    };

    auto it = map.find(FileNameOld);
    if (it == map.cend())
        return false;

    FileNameNew = it->second;

    return true;
}

Qt6HeaderFixes::Qt6HeaderFixes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

void Qt6HeaderFixes::VisitInclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok, clang::StringRef FileName, bool IsAngled,
                        clang::CharSourceRange FilenameRange, const clang::FileEntry *File, clang::StringRef SearchPath,
                        clang::StringRef RelativePath, const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType)
{
    auto current_file = m_sm.getFilename(HashLoc);
    auto main_file = m_sm.getFileEntryForID(m_sm.getMainFileID())->getName();
    if ( current_file != main_file)
        return;

    string newFileName = "";
    if (!newOldHeaderFileMatch(FileName.str(), newFileName))
        return;

    string replacement = "";
    if (IsAngled) {
        replacement = "<";
        replacement += newFileName;
        replacement += ">";
    }
    else {
        replacement = "\"";
        replacement += newFileName;
        replacement += "\"";
    }

    vector<FixItHint> fixits;
    fixits.push_back(FixItHint::CreateReplacement(FilenameRange.getAsRange(), replacement));
    string message = "including ";
    message += FileName;
    emitWarning(FilenameRange.getAsRange().getBegin(), message, fixits);
    return;
}
