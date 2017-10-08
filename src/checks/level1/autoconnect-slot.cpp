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

#include "autoconnect-slot.h"
#include "QtUtils.h"
#include "checkmanager.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"
#include <regex>

using namespace clang;
using namespace std;


AutoConnectSlot::AutoConnectSlot( const std::string &name, ClazyContext *context )
  : CheckBase( name, context )
{
  context->enableAccessSpecifierManager();
}

void AutoConnectSlot::VisitDecl( Decl *decl )
{
  AccessSpecifierManager *a = m_context->accessSpecifierManager;
  if ( !a )
    return;

  auto method = dyn_cast<CXXMethodDecl>( decl );
  if ( !method )
    return;

  if ( method->isThisDeclarationADefinition() && !method->hasInlineBody() ) // Don't warn twice
    return;

  QtAccessSpecifierType specifierType = a->qtAccessSpecifierType( method );

  if ( specifierType != QtAccessSpecifier_Slot )
    return;

  std::string name = method->getNameAsString();

  static regex rx( R"(on_(.*)_(.*))" );
  smatch match;
  if ( !regex_match( name, match, rx ) )
    return;

  std::string objectName = match[1].str();

  CXXRecordDecl *record = method->getParent();
  if ( clang::FieldDecl *field = getClassMember( record, objectName ) )
  {
    QualType type = field->getType();

    if ( QtUtils::isQObject( type ) )
      emitWarning( decl, "Use of autoconnected UI slot: " + name );
  }
}

clang::FieldDecl *AutoConnectSlot::getClassMember( CXXRecordDecl *record, const string &memberName )
{
  if ( !record )
    return nullptr;

  for ( auto field : record->fields() )
  {
    if ( field->getNameAsString() == memberName )
      return field;
  }

  // Also include the base classes
  for ( const CXXBaseSpecifier &base : record->bases() )
  {
    CXXRecordDecl *baseRecord = TypeUtils::recordFromBaseSpecifier( base );
    if ( clang::FieldDecl *field = getClassMember( baseRecord, memberName ) )
    {
      return field;
    }
  }

  return nullptr;
}

REGISTER_CHECK( "autoconnect-slot", AutoConnectSlot, CheckLevel1 )
