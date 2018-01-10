/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

namespace MySQL {
  namespace Forms {

    // Private wrapper class that bridges between the managed dock delegate and the backend.
    class DockingPointDelegateWrapper;
    ref class AppViewDockContent;

    // Class to derive platform dock delegates from.
  public
    ref class ManagedDockDelegate abstract {
      DockingPointDelegateWrapper* wrapper;

    public:
      ManagedDockDelegate(Object ^ represented_object);
      ~ManagedDockDelegate();

      mforms::DockingPointDelegate* get_unmanaged_delegate();

      virtual String ^ get_type(Object ^ represented_object) = 0;
      virtual void dock_view(Object ^ represented_object, AppViewDockContent ^ view, String ^ arg1, int arg2) = 0;
      virtual bool select_view(Object ^ represented_object, AppViewDockContent ^ view) = 0;
      virtual void undock_view(Object ^ represented_object, AppViewDockContent ^ view) = 0;
      virtual void set_view_title(Object ^ represented_object, AppViewDockContent ^ view, String ^ title) = 0;
      virtual Drawing::Size get_size(Object ^ represented_object) = 0;

      virtual int view_count() = 0;
      virtual AppViewDockContent ^ view_at_index(int i) = 0;
      virtual AppViewDockContent ^ selected_view() = 0;
    };
  }
}
