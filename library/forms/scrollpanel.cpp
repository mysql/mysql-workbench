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

#include "mforms/mforms.h"

using namespace mforms;

ScrollPanel::ScrollPanel(ScrollPanelFlags flags) : Container() {
  _spanel_impl = &ControlFactory::get_instance()->_spanel_impl;

  _spanel_impl->create(this, flags);
}

ScrollPanel::~ScrollPanel() {
  set_destroying();
  _spanel_impl->remove(this);
}

void ScrollPanel::add(View* child) {
  cache_view(child);
  _spanel_impl->add(this, child);
  child->show();
}

void ScrollPanel::remove() {
  _spanel_impl->remove(this);
  clear_subviews();
}

void ScrollPanel::set_visible_scrollers(bool vertical, bool horizontal) {
  _spanel_impl->set_visible_scrollers(this, vertical, horizontal);
}

void ScrollPanel::set_autohide_scrollers(bool flag) {
  _spanel_impl->set_autohide_scrollers(this, flag);
}

void ScrollPanel::scroll_to_view(View* child) {
  if (_spanel_impl->scroll_to_view)
    return _spanel_impl->scroll_to_view(this, child);

  throw std::logic_error("ScrollPanel::scroll_to_view: not implemented");
}

base::Rect ScrollPanel::get_content_rect() {
  return _spanel_impl->get_content_rect(this);
}

void ScrollPanel::scroll_to(int x, int y) {
  _spanel_impl->scroll_to(this, x, y);
}
