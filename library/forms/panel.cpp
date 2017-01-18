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

Panel::Panel(PanelType type) {
  _panel_impl = &ControlFactory::get_instance()->_panel_impl;

  _panel_impl->create(this, type);
}

void Panel::set_title(const std::string &title) {
  _panel_impl->set_title(this, title);
}

void Panel::set_back_color(const std::string &color) {
  _panel_impl->set_back_color(this, color);
}

void Panel::set_active(bool flag) {
  _panel_impl->set_active(this, flag);
}

bool Panel::get_active() {
  return _panel_impl->get_active(this);
}

void Panel::add(View *subview) {
  cache_view(subview);
  _panel_impl->add(this, subview);
  subview->show();

#ifdef _WIN32 // XXX this shouldn't be needed here, the plat specific code is supposed to do this
  relayout();
#endif
}

void Panel::remove(View *subview) {
  _panel_impl->remove(this, subview);
  remove_from_cache(subview);

#ifdef _WIN32 // XXX this shouldn't be needed here, the plat specific code is supposed to do this
  relayout();
#endif
}
