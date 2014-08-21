/* 
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "LayerEditorWrapper.h"

using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

LayerEditorWrapper::LayerEditorWrapper(LayerEditorBE *inn)
  : BaseEditorWrapper(inn)
{}

//--------------------------------------------------------------------------------------------------

LayerEditorWrapper::LayerEditorWrapper(MySQL::Grt::GrtManager ^grtm, MySQL::Grt::GrtValue ^arglist)
  : BaseEditorWrapper(
    new ::LayerEditorBE(grtm->get_unmanaged_object(),
      workbench_physical_LayerRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
      )
    )
{
}

//--------------------------------------------------------------------------------------------------

LayerEditorWrapper::~LayerEditorWrapper()
{
  // These wrappers keep a gc pointer to this instance (if assigned);
  set_refresh_partial_ui_handler(nullptr);
  set_refresh_ui_handler(nullptr);

  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

LayerEditorBE *LayerEditorWrapper::get_unmanaged_object()
{
  return static_cast<::LayerEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void LayerEditorWrapper::set_name(String ^name)
{
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String^ LayerEditorWrapper::get_name()
{
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

void LayerEditorWrapper::set_color(String ^color)
{
  get_unmanaged_object()->set_color(NativeToCppString(color));
}

//--------------------------------------------------------------------------------------------------

String^ LayerEditorWrapper::get_color()
{
  return CppStringToNative(get_unmanaged_object()->get_color());
}

//--------------------------------------------------------------------------------------------------
