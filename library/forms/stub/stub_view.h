/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _STUB_VIEW_H_
#define _STUB_VIEW_H_

#include "stub_base.h"

namespace mforms {
  namespace stub {

    class ViewWrapper : public ObjectWrapper {
    protected:
      static void destroy(View *self);

      static int get_width(const mforms::View *self);
      static int get_height(const mforms::View *self);
      static int get_preferred_width(mforms::View *self);
      static int get_preferred_height(mforms::View *self);
      static void set_size(mforms::View *self, int w, int h);
      static void set_min_size(mforms::View *self, int w, int h);
      static void set_padding(View *self, int, int, int, int);

      static int get_x(mforms::View *self);
      static int get_y(mforms::View *self);
      static void set_position(mforms::View *self, int x, int y);
      static std::pair<int, int> client_to_screen(View *self, int, int);
      static std::pair<int, int> screen_to_client(View *self, int, int);

      static void show(mforms::View *self, bool show);
      static bool is_shown(mforms::View *self);

      static void set_tooltip(mforms::View *self, const std::string &text);
      static void set_name(mforms::View *view, const std::string &name);
      static void set_font(mforms::View *view, const std::string &font);

      static void set_enabled(mforms::View *self, bool flag);
      static bool is_enabled(View *self);
      static void relayout(mforms::View *view);
      static void set_needs_repaint(mforms::View *view);

      static void suspend_layout(View *self, bool);
      static void set_front_color(mforms::View *self, const std::string &color);
      static std::string get_front_color(View *self);
      static void set_back_color(mforms::View *self, const std::string &color);
      static std::string get_back_color(View *self);
      static void set_back_image(mforms::View *self, const std::string &path, mforms::Alignment layout);

      static void flush_events(View *self);

      static void focus(View *self);

      static void register_drop_formats(View *self, DropDelegate *target, const std::vector<std::string> &);
      static DragOperation drag_text(View *self, DragDetails details, const std::string &text);
      static DragOperation drag_data(View *self, DragDetails details, void *data, const std::string &format);

      ViewWrapper(mforms::View *view);

      virtual int get_preferred_width();
      virtual int get_preferred_height();
      virtual void set_size(int width, int height);
      void size_changed();

    public:
      static void init();
    };
  };
};

#endif
