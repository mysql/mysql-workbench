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

//#include "wb_config.h"
#ifdef COMMERCIAL_CODE

#include "image_cache.h"
#include "validation_panel.h"
#include <gtkmm/treeview.h>
#include "base/string_utilities.h"

ValidationPanel::ValidationPanel()
  : Gtk::Box(Gtk::ORIENTATION_VERTICAL), _label_box(0), _label(_("<small>Validations</small>")) {
  _label.set_use_markup(true);
  _model = ListModelWrapper::create(&_be, &_tv, "ValidationModel");

  _model->model().append_string_column(bec::ValidationMessagesBE::Description, _("Description"), RO, WITH_ICON);

  _tv.set_fixed_height_mode(false);
  _tv.set_model(_model);

  add(_tv);

  show_all();

  std::vector<Gtk::CellRenderer*> cells = _tv.get_column(0)->get_cells();
  Gtk::CellRendererText* ct = dynamic_cast<Gtk::CellRendererText*>(cells[1]);
  ct->property_wrap_width() = 120;
  ct->property_wrap_mode() = Pango::WRAP_WORD;

  _tv.signal_size_allocate().connect(sigc::mem_fun(this, &ValidationPanel::size_request_slot));

  scoped_connect(_be.tree_changed_signal(), sigc::mem_fun(this, &ValidationPanel::refresh));
}

void ValidationPanel::refresh(const bec::NodeId& node, int ocount) {
  _tv.unset_model();
  _tv.set_model(_model);

  if (_be.count() > 0) {
    _icon.set(ImageCache::get_instance()->image_from_filename("mini_warning.png"));
    _icon.show();
  } else
    _icon.hide();
}

void ValidationPanel::size_request_slot(Gtk::Allocation& req) {
  std::vector<Gtk::CellRenderer*> cells = _tv.get_column(0)->get_cells();
  Gtk::CellRendererText* ct = dynamic_cast<Gtk::CellRendererText*>(cells[1]);
  ct->property_wrap_width() = req.get_width() - 20;
}

Gtk::Box* ValidationPanel::notebook_label(Gtk::Box* lbl) {
  if (!lbl)
    lbl = new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL);

  _label_box = lbl;
  _label_box->add(_icon);
  _label_box->add(_label);
  _label_box->show_all();

  return lbl;
}

#endif
