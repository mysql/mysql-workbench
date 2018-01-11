/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#pragma once

namespace wb {
  class ModelDiagramForm;
}

namespace MySQL {
  namespace Workbench {

  public
    ref class ModelDiagramFormWrapper : public MySQL::Base::UIForm {
      MySQL::GUI::Mdc::BaseWindowsCanvasView ^ mini_view;

    public:
      ModelDiagramFormWrapper(wb::ModelDiagramForm* inn);
      ~ModelDiagramFormWrapper();

      wb::ModelDiagramForm* get_unmanaged_object();

      void OnMouseMove(System::Windows::Forms::MouseEventArgs ^ e, int X, int Y, System::Windows::Forms::Keys keystate,
                       System::Windows::Forms::MouseButtons buttons);
      void OnMouseDown(System::Windows::Forms::MouseEventArgs ^ e, int X, int Y, System::Windows::Forms::Keys keystate,
                       System::Windows::Forms::MouseButtons buttons);
      void OnMouseUp(System::Windows::Forms::MouseEventArgs ^ e, int X, int Y, System::Windows::Forms::Keys keystate,
                     System::Windows::Forms::MouseButtons buttons);
      void OnMouseDoubleClick(System::Windows::Forms::MouseEventArgs ^ e, int X, int Y,
                              System::Windows::Forms::Keys keystate, System::Windows::Forms::MouseButtons buttons);
      void OnKeyDown(System::Windows::Forms::KeyEventArgs ^ e, System::Windows::Forms::Keys keystate);
      void OnKeyUp(System::Windows::Forms::KeyEventArgs ^ e, System::Windows::Forms::Keys keystate);

      String ^ get_tool_cursor();

      bool accepts_drop(int x, int y, System::Windows::Forms::IDataObject ^ data);
      bool accepts_drop(int x, int y, String ^ type, String ^ text);
      bool perform_drop(int x, int y, System::Windows::Forms::IDataObject ^ data);
      bool perform_drop(int x, int y, String ^ type, String ^ text);

      void set_closed(bool flag);
      bool is_closed();
      void close();
      void setup_mini_view(MySQL::GUI::Mdc::BaseWindowsCanvasView ^ view);
      void update_mini_view_size(int w, int h);
      void update_options_toolbar();

      double get_zoom();
      void set_zoom(double zoom);

      String ^ get_title();
      System::Windows::Forms::ToolStrip ^ get_tools_toolbar();
      System::Windows::Forms::ToolStrip ^ get_options_toolbar();
      Aga::Controls::Tree::TreeViewAdv ^ get_layer_tree();
      Aga::Controls::Tree::TreeViewAdv ^ get_catalog_tree();
    };

  } // namespace Workbench
} // namespace MySQL
