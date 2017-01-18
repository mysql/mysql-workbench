/*
* Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace MySQL {
  namespace Base {
  public
    interface class IRecordsetView {
      enum class ColumnHeaderIndicator { NoOrder = 0, OrderDesc = -1, OrderAsc = 1 };
      System::Windows::Forms::Control ^ control();

      delegate void ColumnResizeCallback(int column);
      delegate System::Windows::Forms::ContextMenuStrip ^ ColumnHeaderRightClickCallback(int column);

      void set_column_resize_callback(ColumnResizeCallback ^ callback);
      void set_column_header_right_click_callback(ColumnHeaderRightClickCallback ^ callback);

      int get_column_count();
      int get_column_width(int column);
      void set_column_width(int column, int width);
      int current_cell_row();
      int current_cell_column();
      void set_current_cell(int row, int column);
      void update_columns();

      void set_font(System::String ^ font, float size, System::Drawing::FontStyle style);
      void set_column_header_indicator(int column, ColumnHeaderIndicator order);
    };
  }
}
