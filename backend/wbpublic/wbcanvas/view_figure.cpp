/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "view_figure.h"

using namespace wbfig;
using namespace base;

View::View(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self)
  : BaseFigure(layer, hub, self), _title(layer, hub, this, false) {
  _title.set_icon(mdc::ImageManager::get_instance()->get_image("workbench.physical.ViewFigure.16x16.png"));

  set_allowed_resizing(false, false);
  set_accepts_focus(true);
  set_accepts_selection(true);

  set_background_corners(mdc::CAll, 8.0);

  _title.set_rounded(mdc::CAll);
  _title.set_draggable(true);
  _title.set_expanded(true);
  _title.set_has_shadow(true);
  _title.set_title("View");
  _title.set_font(mdc::FontSpec("Helvetica", mdc::SNormal, mdc::WBold, 12));
  _title.set_color(Color(0.59, 0.75, 0.85));

  add(&_title, false, false, true);
}

View::~View() {
}

void View::set_title(const std::string &title) {
  _title.set_title(title);
}

void View::set_color(const Color &color) {
  _title.set_color(color);
  set_needs_render();
}

void View::set_title_font(const mdc::FontSpec &font) {
  _title.set_font(font);
  set_needs_render();
}
