/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "Grt.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "GrtManager.h"

#include "DBObjectEditorWrapper.h"
#include "grts/structs.workbench.physical.h"

#include "RoutineEditorBE.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RoutineEditorBE::RoutineEditorBE(::bec::RoutineEditorBE *inn)
  : DBObjectEditorWrapper(inn)
{
}

//--------------------------------------------------------------------------------------------------

::bec::RoutineEditorBE *RoutineEditorBE::get_unmanaged_object()
{
  return static_cast<::bec::RoutineEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

String^ RoutineEditorBE::get_sql()
{
  return CppStringToNative(get_unmanaged_object()->get_sql());
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorBE::set_sql(String ^query, bool sync)
{
  get_unmanaged_object()->set_sql(NativeToCppString(query), sync);
}

//--------------------------------------------------------------------------------------------------

String^ RoutineEditorBE::get_name()
{
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorBE::set_name(String ^name)
{
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String^ RoutineEditorBE::get_comment()
{
  return CppStringToNative(get_unmanaged_object()->get_comment());
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorBE::set_comment(String ^comment)
{
  get_unmanaged_object()->set_comment(NativeToCppString(comment));
}

//--------------------------------------------------------------------------------------------------

String^ RoutineEditorBE::get_formatted_sql_for_editing([Out] int %cursor_pos)
{ 
  int cpos= cursor_pos; 
  String ^res= CppStringToNative(get_unmanaged_object()->get_formatted_sql_for_editing(cpos)); 
  cursor_pos= cpos;
  return res;
}

//--------------------------------------------------------------------------------------------------
