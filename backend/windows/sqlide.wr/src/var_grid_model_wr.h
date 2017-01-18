/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
  namespace Grt {

  public
    ref class VarGridModelWrapper : public MySQL::Grt::GridModelWrapper {
      typedef ManagedRef<::VarGridModel> ^ Ref;

    private:
      Ref _ref;

      boost::signals2::connection *_refresh_ui_connection;
      DelegateSlot0<void, void> ^ _refresh_ui_cb;
      DelegateSlot0<void, void> ^ _update_selection;
      DelegateSlot0<void, void> ^ _rows_changed;

    public:
      VarGridModelWrapper(Ref ref);
      VarGridModelWrapper(IntPtr nref_ptr);

      ~VarGridModelWrapper();

      Ref ref() {
        return _ref;
      }

      void refresh_ui_cb(DelegateSlot0<void, void>::ManagedDelegate ^ cb);
      void set_update_selection_delegate(DelegateSlot0<void, void>::ManagedDelegate ^ selection);
      void set_rows_changed(DelegateSlot0<void, void>::ManagedDelegate ^ update);

      int edited_field_row();
      int edited_field_column();
    };

  }; // namespace Grt
};   // namespace MySQL
