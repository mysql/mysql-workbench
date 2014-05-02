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

#ifndef __IMAGE_EDITOR_H__
#define __IMAGE_EDITOR_H__

#include "wb_editor_image.h"
#include "GrtTemplates.h"


#pragma make_public(::ImageEditorBE)

using namespace MySQL::Grt;
using namespace System;
using namespace System::Collections::Generic;
using namespace Runtime::InteropServices; // Needed for the [Out] keyword.

namespace MySQL {
namespace Grt {

public ref class ImageEditorBE : public BaseEditorWrapper
{
protected:
  ImageEditorBE(::ImageEditorBE *inn)
    : BaseEditorWrapper(inn)
  {}

public:
  ImageEditorBE::ImageEditorBE(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
  : BaseEditorWrapper(
      new ::ImageEditorBE(grtm->get_unmanaged_object(), 
      workbench_model_ImageFigureRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
          )
      )
  {
  }

  ::ImageEditorBE *get_unmanaged_object()
  { return static_cast<::ImageEditorBE *>(inner); }

  void set_filename(String ^text)
  {
    get_unmanaged_object()->set_filename(NativeToCppString(text));
  }

  String^ get_filename()
  {
    return CppStringToNative(get_unmanaged_object()->get_filename());
  }

  String^ get_attached_image_path()
  {
    return CppStringToNative(get_unmanaged_object()->get_attached_image_path());
  }

  void get_size([Out] int %w, [Out] int %h)
  {
    int ww, hh;
    get_unmanaged_object()->get_size(ww, hh);
    w= ww;
    h= hh;
  }

  void set_size(int w, int h)
  {
    get_unmanaged_object()->set_size(w, h);
  }

  void set_width(int w)
  {
    get_unmanaged_object()->set_width(w);
  }

  void set_height(int h)
  {
    get_unmanaged_object()->set_height(h);
  }

  bool get_keep_aspect_ratio()
  {
    return get_unmanaged_object()->get_keep_aspect_ratio();
  }

  void set_keep_aspect_ratio(bool flag)
  {
    get_unmanaged_object()->set_keep_aspect_ratio(flag);
  }
};

} // namespace Grt
} // namespace MySQL

#endif // __IMAGE_EDITOR_H__