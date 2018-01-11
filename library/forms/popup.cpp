/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

//--------------------------------------------------------------------------------------------------

Popup::Popup(PopupStyle style) {
  _popup_impl = &ControlFactory::get_instance()->_popup_impl;

  _popup_impl->create(this, style);
}

//--------------------------------------------------------------------------------------------------

Popup::~Popup() {
  _popup_impl->destroy(this);
}

//--------------------------------------------------------------------------------------------------

void Popup::set_needs_repaint() {
  _popup_impl->set_needs_repaint(this);
}

//--------------------------------------------------------------------------------------------------

void Popup::set_size(int width, int height) {
  _popup_impl->set_size(this, width, height);
}

//--------------------------------------------------------------------------------------------------

int Popup::show(int spot_x, int spot_y) {
  return _popup_impl->show(this, spot_x, spot_y);
}

//--------------------------------------------------------------------------------------------------

base::Rect Popup::get_content_rect() {
  return _popup_impl->get_content_rect(this);
}

//--------------------------------------------------------------------------------------------------

/**
 * Some types of popup's act like modal windows, so we need a way to close them out-of-order.
 * This result is usually set by internal handling (e.g. on mouse click or key press). But sometimes it is
 * necessary to set it from outside to finish the modal mode. The result set here is returned by
 * the show() call and only has an effect while the popup is shown.
 */
void Popup::set_modal_result(int result) {
  _popup_impl->set_modal_result(this, result);
}

//--------------------------------------------------------------------------------------------------

void mforms::Popup::closed() {
  _on_close();
}

//--------------------------------------------------------------------------------------------------
