/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __LAYEREDITOR_H__
#define __LAYEREDITOR_H__

#include "wb_editor_layer.h"
#include "GrtTemplates.h"


#pragma make_public(::LayerEditorBE)

#ifdef _MSC_VER
using namespace MySQL::Grt;
using namespace System;
using namespace System::Collections::Generic;
#endif

namespace MySQL {
namespace Grt {

public ref class LayerEditorBE : public BaseEditorWrapper
{
protected:
  LayerEditorBE(::LayerEditorBE *inn)
    : BaseEditorWrapper(inn)
  {}

public:
  LayerEditorBE::LayerEditorBE(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
  : BaseEditorWrapper(
      new ::LayerEditorBE(grtm->get_unmanaged_object(), 
      workbench_physical_LayerRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
          )
      )
  {
  }

  ::LayerEditorBE *get_unmanaged_object()
  { return static_cast<::LayerEditorBE *>(inner); }

  void set_name(String ^name)
  {
    get_unmanaged_object()->set_name(NativeToCppString(name));
  }

  String^ get_name()
  {
    return CppStringToNative(get_unmanaged_object()->get_name());
  }

  void set_color(String ^color)
  {
    get_unmanaged_object()->set_color(NativeToCppString(color));
  }

  String^ get_color()
  {
    return CppStringToNative(get_unmanaged_object()->get_color());
  }
};

} // namespace Grt
} // namespace MySQL

#endif // __LAYEREDITOR_H__