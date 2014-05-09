/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "gui_plugin_base.h"
#include "grt/plugin_manager.h"

GUIPluginBase::GUIPluginBase(grt::Module *module)
  : _module(module), _grt(module->get_grt())
{
}

GUIPluginBase::GUIPluginBase(grt::GRT *grt)
  : _module(NULL), _grt(grt)
{
}

GUIPluginBase::~GUIPluginBase()
{
  grtm()->get_plugin_manager()->forget_gui_plugin_handle(reinterpret_cast<NativeHandle>(this));
}
