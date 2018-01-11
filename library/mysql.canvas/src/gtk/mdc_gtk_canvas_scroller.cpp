/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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


#include "mdc_gtk_canvas_scroller.h"
#include "mdc_gtk_canvas_view.h"

using namespace mdc;

GtkCanvasScroller::GtkCanvasScroller() : Gtk::Table(2, 2) {
  attach(_vscroll, 1, 2, 0, 1, Gtk::FILL, Gtk::FILL | Gtk::EXPAND);
  attach(_hscroll, 0, 1, 1, 2, Gtk::FILL | Gtk::EXPAND, Gtk::FILL);
  show_all();

  _hscroll.get_adjustment()->set_page_increment(50.0);
  _hscroll.get_adjustment()->set_step_increment(5.0);

  _vscroll.get_adjustment()->set_page_increment(50.0);
  _vscroll.get_adjustment()->set_step_increment(5.0);
}

void GtkCanvasScroller::add(GtkCanvas &canvas) {
  attach(canvas, 0, 1, 0, 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::EXPAND);
  canvas.show();
  canvas.set_vadjustment(_vscroll.get_adjustment());
  canvas.set_hadjustment(_hscroll.get_adjustment());
}

Glib::RefPtr<Gtk::Adjustment> GtkCanvasScroller::get_hadjustment() {
  return _hscroll.get_adjustment();
}

Glib::RefPtr<Gtk::Adjustment> GtkCanvasScroller::get_vadjustment() {
  return _vscroll.get_adjustment();
}
