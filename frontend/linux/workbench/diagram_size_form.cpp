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


#include "diagram_size_form.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"
#include "model/wb_diagram_options.h"
#include "gtk/mdc_gtk_canvas_view.h"
#include <gtkmm/spinbutton.h>
#include <gtkmm/frame.h>

DiagramSizeForm::DiagramSizeForm(GtkDialog *gobj, Glib::RefPtr<Gtk::Builder> builder)
  : Gtk::Dialog(gobj), _xml(builder), _canvas(nullptr) {
  _be = 0;
}

DiagramSizeForm::~DiagramSizeForm() {
  delete _be;
}

void DiagramSizeForm::spin_changed() {
  Gtk::SpinButton *spin;
  _xml->get_widget("spinbutton1", spin);
  _be->set_xpages(spin->get_value());

  _xml->get_widget("spinbutton2", spin);
  _be->set_ypages(spin->get_value());
}

void DiagramSizeForm::changed() {
  Gtk::SpinButton *spin;
  _xml->get_widget("spinbutton1", spin);
  spin->set_value(_be->get_xpages());

  _xml->get_widget("spinbutton2", spin);
  spin->set_value(_be->get_ypages());
}

void DiagramSizeForm::realize_be() {
  _be = wb::WBContextUI::get()->create_diagram_options_be(_canvas->get_canvas());
  _be->update_size();

  scoped_connect(_be->signal_changed(), sigc::mem_fun(this, &DiagramSizeForm::changed));

  Gtk::SpinButton *spin;
  _xml->get_widget("spinbutton1", spin);
  spin->set_value(_be->get_xpages());
  spin->signal_changed().connect(sigc::mem_fun(this, &DiagramSizeForm::spin_changed));

  _xml->get_widget("spinbutton2", spin);
  spin->set_value(_be->get_ypages());
  spin->signal_changed().connect(sigc::mem_fun(this, &DiagramSizeForm::spin_changed));

  Gtk::Entry *entry = 0;
  _xml->get_widget("name_entry", entry);
  entry->set_text(_be->get_name());

  Gtk::Button *btn;
  _xml->get_widget("button1", btn); // ok
  btn->signal_clicked().connect(sigc::mem_fun(this, &DiagramSizeForm::ok_clicked));
}

void DiagramSizeForm::ok_clicked() {
  Gtk::Entry *entry = 0;
  _xml->get_widget("name_entry", entry);
  _be->set_name(entry->get_text());

  _be->commit();
}

void DiagramSizeForm::init() {
  Gtk::Frame *frame = 0;

  _xml->get_widget("frame", frame);

  _canvas = Gtk::manage(new mdc::GtkCanvas(mdc::GtkCanvas::BufferedXlibCanvasType));
  frame->add(*_canvas);
  _canvas->show();

  _canvas->signal_realize().connect_notify(sigc::mem_fun(this, &DiagramSizeForm::realize_be));
}

DiagramSizeForm *DiagramSizeForm::create() {
  Glib::RefPtr<Gtk::Builder> ui =
    Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path("diagram_size_form.glade"));

  DiagramSizeForm *panel = 0;

  ui->get_widget_derived<DiagramSizeForm>("dialog", panel);

  if (panel)
    panel->init();

  return panel;
}
