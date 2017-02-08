/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "wb_editor_image.h"
#include "GrtTemplates.h"

#pragma make_public(ImageEditorBE)

namespace MySQL {
  namespace Grt {

  public
    ref class ImageEditorWrapper : public BaseEditorWrapper {
    protected:
      ImageEditorWrapper(::ImageEditorBE *inn);

    public:
      ImageEditorWrapper::ImageEditorWrapper(MySQL::Grt::GrtValue ^ arglist);
      ~ImageEditorWrapper();

      ImageEditorBE *get_unmanaged_object();
      void set_filename(String ^ text);
      String ^ get_filename();
      String ^ get_attached_image_path();
      void get_size([Out] int % w, [Out] int % h);
      void set_size(int w, int h);
      void set_width(int w);
      void set_height(int h);
      bool get_keep_aspect_ratio();
      void set_keep_aspect_ratio(bool flag);
    };

  } // namespace Grt
} // namespace MySQL
