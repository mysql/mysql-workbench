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

#ifndef _MDC_GTK_CANVAS_SCROLLER_H_
#define _MDC_GTK_CANVAS_SCROLLER_H_

#include <gtkmm/table.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/adjustment.h>

namespace mdc {

  class GtkCanvas;

  class GtkCanvasScroller : public Gtk::Table {
    Gtk::HScrollbar _hscroll;
    Gtk::VScrollbar _vscroll;

  public:
    GtkCanvasScroller();

    Glib::RefPtr<Gtk::Adjustment> get_hadjustment();
    Glib::RefPtr<Gtk::Adjustment> get_vadjustment();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual" // The GtkCanvas is descendant of Gtk::Layout
    void add(GtkCanvas &canvas);                      //
#pragma GCC diagnostic pop
  };
};

#endif /* _MDC_GTK_CANVAS_SCROLLER_H_ */
