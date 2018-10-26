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

#include "navigator_box.h"
#include "image_cache.h"

#include <gtkmm/image.h>

#include "model/wb_model_diagram_form.h"
#include "base/string_utilities.h"

using base::strfmt;

static int zoom_levels[] = {200, 150, 100, 95, 90, 85, 80, 75, 70, 60, 50, 40, 30, 20, 10};

NavigatorBox::NavigatorBox()
  : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2),
    _model(0),
    _canvas(mdc::GtkCanvas::XlibCanvasType),
    _combo(true),
    _changing_zoom(false) {
  Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));

  _canvas.signal_size_allocate().connect(sigc::mem_fun(this, &NavigatorBox::size_change));
  _canvas.signal_realize().connect(sigc::mem_fun(this, &NavigatorBox::canvas_realize));
  pack_start(_canvas, true, true);
  pack_start(*hbox, false, false);

  Gtk::Image *image1 =
    Gtk::manage(new Gtk::Image(ImageCache::get_instance()->image_from_filename("navigator_zoom_in.png")));
  Gtk::Image *image2 =
    Gtk::manage(new Gtk::Image(ImageCache::get_instance()->image_from_filename("navigator_zoom_out.png")));

  _combo.set_name("Zoom");
  _combo.set_size_request(60, -1);
  _slider.set_draw_value(false);
  _slider.set_range(10, 200);
  _slider.signal_value_changed().connect(sigc::mem_fun(this, &NavigatorBox::slider_changed));
  _combo.signal_changed().connect(sigc::bind(sigc::mem_fun(this, &NavigatorBox::combo_changed), false));
  _combo.get_entry()->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &NavigatorBox::combo_changed), true));
  _combo.get_entry()->signal_focus_out_event().connect(
    sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(this, &NavigatorBox::combo_changed), true)), false));

  for (unsigned int i = 0; i < sizeof(zoom_levels) / sizeof(int); i++)
    _combo.append(strfmt("%i", zoom_levels[i]));
  _combo.set_active_text("100");

  _zoom_in.add(*image1);
  _zoom_out.add(*image2);
  _zoom_in.set_relief(Gtk::RELIEF_NONE);
  _zoom_in.set_focus_on_click(false);
  _zoom_out.set_relief(Gtk::RELIEF_NONE);
  _zoom_out.set_focus_on_click(false);

  hbox->pack_start(_zoom_out, false, false);
  hbox->pack_start(_slider, true, true);
  hbox->pack_start(_zoom_in, false, false);
  hbox->pack_start(_combo, false, false);

  show_all();
}

void NavigatorBox::set_model(wb::ModelDiagramForm *model) {
  _model = model;

  _zoom_in.signal_clicked().connect(sigc::mem_fun(_model, &wb::ModelDiagramForm::zoom_in));
  _zoom_out.signal_clicked().connect(sigc::mem_fun(_model, &wb::ModelDiagramForm::zoom_out));

  //  g_assert(_model->get_view());
  _model->setup_mini_view(_canvas.get_canvas());
  _model->update_mini_view_size(_canvas.get_width(), _canvas.get_height());
}

void NavigatorBox::size_change(Gtk::Allocation &alloc) {
  if (_model)
    _model->update_mini_view_size(_canvas.get_width(), _canvas.get_height());
}

void NavigatorBox::canvas_realize() {
  // we need to add additional reference as gtk3 is releasing it in different order so there's a crash
  if (_canvas.get_canvas())
    cairo_reference(_canvas.get_canvas()->cairoctx()->get_cr());
}

void NavigatorBox::refresh() {
  int value = 100;

  //  if (is_realized())
  //    realized();
  if (_changing_zoom || !_model)
    return;

  _changing_zoom = true;
  value = (int)(_model->get_zoom() * 100);

  _slider.set_value(value);
  _combo.get_entry()->set_text(strfmt("%i", value));

  _slider.set_sensitive(_model != 0);
  _combo.set_sensitive(_model != 0);

  _changing_zoom = false;
}

void NavigatorBox::slider_changed() {
  if (_model && !_changing_zoom) {
    _changing_zoom = true;
    int value = (int)_slider.get_value();
    _model->set_zoom(value / 100.0);
    refresh();
    _changing_zoom = false;
  }
}

void NavigatorBox::combo_changed(bool force_update) {
  if (_model && !_changing_zoom) {
    _changing_zoom = true;
    if (force_update || !_combo.get_entry()->has_focus()) {
      _model->set_zoom(base::atoi<int>(_combo.get_entry()->get_text(), 0) / 100.0);

      refresh();
    }
    _changing_zoom = false;
  }
}
