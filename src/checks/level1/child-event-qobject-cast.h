/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CHILD_EVENT_QOBJECT_CAST_H
#define CLAZY_CHILD_EVENT_QOBJECT_CAST_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Decl;
} // namespace clang

/**
 * See README-child-event-qobject-cast for more info.
 */
class ChildEventQObjectCast : public CheckBase
{
public:
    explicit ChildEventQObjectCast(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
};

#endif
