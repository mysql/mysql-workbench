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

#include "wb_editor_layer.h"
#include "GrtTemplates.h"

#pragma make_public(LayerEditorBE)

namespace MySQL {
  namespace Grt {

  public
    ref class LayerEditorWrapper : public BaseEditorWrapper {
    protected:
      LayerEditorWrapper(::LayerEditorBE *inn);

    public:
      LayerEditorWrapper::LayerEditorWrapper(MySQL::Grt::GrtValue ^ arglist);
      ~LayerEditorWrapper();

      LayerEditorBE *get_unmanaged_object();
      void set_name(String ^ name);
      String ^ get_name();
      void set_color(String ^ color);
      String ^ get_color();
    };

  } // namespace Grt
} // namespace MySQL
