/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#include "virtual-signal.h"
#include "QtUtils.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;


VirtualSignal::VirtualSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}


void VirtualSignal::VisitDecl(Decl *stmt)
{
    auto method = dyn_cast<CXXMethodDecl>(stmt);
    if (!method || !method->isVirtual())
        return;

    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager)
        return;

    QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
    if (qst == QtAccessSpecifier_Signal) {

        for (auto m : method->overridden_methods()) {

            if (auto baseClass = m->getParent()) {
                if (!clazy::isQObject(baseClass)) {
                    // It's possible that the signal is overriding a method from a non-QObject base class
                    // if the derived class inherits both QObject and some other interface.
                    return;
                }
            }
        }

        emitWarning(method, "signal is virtual");
    }
}
