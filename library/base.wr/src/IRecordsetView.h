/*
* Copyright (c) 2010, 2014 Oracle and/or its affiliates. All rights reserved.
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
    public interface class IRecordsetView
    {
      System::Windows::Forms::Control ^control();

      int get_column_count();
      int get_column_width(int column);
      void set_column_width(int column, int width);
      int current_cell_row();
      int current_cell_column();
      void set_current_cell(int row, int column);
    };
  }
}
