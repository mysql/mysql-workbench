/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "../stub_view.h"

namespace mforms {
  namespace stub {

    ViewWrapper::ViewWrapper(mforms::View *view) : ObjectWrapper(view) {
    }

    void ViewWrapper::show(mforms::View *self, bool show) {
    }

    bool ViewWrapper::is_shown(mforms::View *self) {
      return false;
    }

    void ViewWrapper::set_tooltip(mforms::View *self, const std::string &text) {
    }

    int ViewWrapper::get_width(const mforms::View *self) {
      return 0;
    }

    int ViewWrapper::get_height(const mforms::View *self) {
      return 0;
    }

    int ViewWrapper::get_preferred_width(mforms::View *self) {
      return 0;
    }

    int ViewWrapper::get_preferred_width() {
      return 0;
    }

    int ViewWrapper::get_preferred_height(mforms::View *self) {
      return 0;
    }

    int ViewWrapper::get_preferred_height() {
      return 0;
    }

    int ViewWrapper::get_x(const mforms::View *self) {
      return 0;
    }

    int ViewWrapper::get_y(const mforms::View *self) {
      return 0;
    }

    void ViewWrapper::set_size(mforms::View *self, int w, int h) {
    }

    void ViewWrapper::set_size(int width, int height) {
    }

    void ViewWrapper::set_min_size(mforms::View *self, int width, int height) {
    }

    void ViewWrapper::set_position(mforms::View *self, int x, int y) {
    }

    void ViewWrapper::set_enabled(mforms::View *self, bool flag) {
    }

    bool ViewWrapper::is_enabled(mforms::View *self) {
      return true;
    }

    void ViewWrapper::set_name(mforms::View *view, const std::string &name) {
    }

    void ViewWrapper::set_font(mforms::View *view, const std::string &font) {
    }

    void ViewWrapper::relayout(mforms::View *view) {
    }

    void ViewWrapper::set_needs_repaint(mforms::View *view) {
    }

    void ViewWrapper::size_changed() {
    }

    void ViewWrapper::suspend_layout(mforms::View *self, bool) {
    }

    void ViewWrapper::set_front_color(mforms::View *self, const std::string &color) {
    }

    std::string ViewWrapper::get_front_color(mforms::View *self) {
      return "#000000";
    }

    void ViewWrapper::set_back_color(mforms::View *self, const std::string &color) {
    }

    std::string ViewWrapper::get_back_color(mforms::View *self) {
      return "#FFFFFF";
    }

    void ViewWrapper::set_back_image(mforms::View *self, const std::string &path, mforms::Alignment layout) {
    }

    void ViewWrapper::flush_events(mforms::View *self) {
    }

    void ViewWrapper::focus(mforms::View *self) {
    }

    void ViewWrapper::destroy(mforms::View *self) {
    }

    void ViewWrapper::set_padding(mforms::View *self, int left, int top, int right, int bottom) {
    }

    std::pair<int, int> ViewWrapper::client_to_screen(mforms::View *self, int x, int y) {
      return std::make_pair(0, 0);
    }

    std::pair<int, int> ViewWrapper::screen_to_client(mforms::View *self, int x, int y) {
      return std::make_pair(0, 0);
    }

    void ViewWrapper::register_drop_formats(View *self, DropDelegate *target, const std::vector<std::string> &) {
    }

    DragOperation ViewWrapper::drag_text(View *self, DragDetails details, const std::string &text) {
      return mforms::DragOperationNone;
    }

    DragOperation ViewWrapper::drag_data(View *self, DragDetails details, void *data, const std::string &format) {
      return mforms::DragOperationNone;
    }

    void ViewWrapper::init() {
      mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

      f->_view_impl.destroy = &ViewWrapper::destroy;

      f->_view_impl.get_width = &ViewWrapper::get_width;
      f->_view_impl.get_height = &ViewWrapper::get_height;
      f->_view_impl.get_preferred_width = &ViewWrapper::get_preferred_width;
      f->_view_impl.get_preferred_height = &ViewWrapper::get_preferred_height;
      f->_view_impl.set_size = &ViewWrapper::set_size;
      f->_view_impl.set_min_size = &ViewWrapper::set_min_size;
      f->_view_impl.set_padding = &ViewWrapper::set_padding;

      f->_view_impl.get_x = &ViewWrapper::get_x;
      f->_view_impl.get_y = &ViewWrapper::get_y;
      f->_view_impl.set_position = &ViewWrapper::set_position;
      f->_view_impl.client_to_screen = &ViewWrapper::client_to_screen;
      f->_view_impl.screen_to_client = &ViewWrapper::screen_to_client;

      f->_view_impl.show = &ViewWrapper::show;
      f->_view_impl.is_shown = &ViewWrapper::is_shown;

      f->_view_impl.set_tooltip = &ViewWrapper::set_tooltip;
      f->_view_impl.set_name = &ViewWrapper::set_name;
      f->_view_impl.set_font = &ViewWrapper::set_font;

      f->_view_impl.set_enabled = &ViewWrapper::set_enabled;
      f->_view_impl.is_enabled = &ViewWrapper::is_enabled;
      f->_view_impl.relayout = &ViewWrapper::relayout;
      f->_view_impl.set_needs_repaint = &ViewWrapper::set_needs_repaint;

      f->_view_impl.suspend_layout = &ViewWrapper::suspend_layout;
      f->_view_impl.set_front_color = &ViewWrapper::set_front_color;
      f->_view_impl.get_front_color = &ViewWrapper::get_front_color;
      f->_view_impl.set_back_color = &ViewWrapper::set_back_color;
      f->_view_impl.get_back_color = &ViewWrapper::get_back_color;
      f->_view_impl.set_back_image = &ViewWrapper::set_back_image;

      f->_view_impl.flush_events = &ViewWrapper::flush_events;
      f->_view_impl.focus = &ViewWrapper::focus;

      f->_view_impl.register_drop_formats = &ViewWrapper::register_drop_formats;
      f->_view_impl.drag_text = &ViewWrapper::drag_text;
      f->_view_impl.drag_data = &ViewWrapper::drag_data;
    };
  };
};
