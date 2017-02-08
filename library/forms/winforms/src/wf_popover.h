/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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
  namespace Forms {

    ref class PopoverControl;

  public
    class PopoverWrapper : public ObjectWrapper {
    private:
      boost::signals2::connection _track_connection;
      bool mouse_left_tracked_object();

    protected:
      PopoverWrapper(mforms::Popover *backend);

      static bool create(mforms::Popover *backend, mforms::PopoverStyle style);
      static void destroy(mforms::Popover *backend);
      static void set_content(mforms::Popover *backend, mforms::View *content);
      static void set_size(mforms::Popover *backend, int width, int height);
      static void show(mforms::Popover *backend, int spot_x, int spot_y, mforms::StartPosition position);
      static void show_and_track(mforms::Popover *backend, mforms::View *owner, int spot_x, int spot_y,
                                 mforms::StartPosition position);
      static base::Rect get_content_rect(mforms::Popover *backend);
      static void close(mforms::Popover *backend);

    public:
      static void init();
    };
  };
};
