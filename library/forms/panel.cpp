/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _MSC_VER // XXX this shouldn't be needed here, the plat specific code is supposed to do this
  relayout();
#endif
}

void Panel::remove(View *subview) {
  _panel_impl->remove(this, subview);
  remove_from_cache(subview);

#ifdef _MSC_VER // XXX this shouldn't be needed here, the plat specific code is supposed to do this
  relayout();
#endif
}
