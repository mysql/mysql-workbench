/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

    ref class PopoverControl;

  public
    class PopoverWrapper : public ObjectWrapper {
    private:
      boost::signals2::connection _track_connection;
      bool mouse_left_tracked_object();

    protected:
      PopoverWrapper(mforms::Popover *backend);

      static bool create(mforms::Popover *backend, mforms::View *relative, mforms::PopoverStyle style);
      static void destroy(mforms::Popover *backend);
      static void set_content(mforms::Popover *backend, mforms::View *content);
      static void set_size(mforms::Popover *backend, int width, int height);
      static void show(mforms::Popover *backend, int spot_x, int spot_y, mforms::StartPosition position);
      static void show_and_track(mforms::Popover *backend, mforms::View *owner, int spot_x, int spot_y,
                                 mforms::StartPosition position);
      static base::Rect get_content_rect(mforms::Popover *backend);
      static void setName(mforms::Popover *backend, const std::string &name);
      static void close(mforms::Popover *backend);

    public:
      static void init();
    };
  };
};
