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

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/liststore.h>
#include "documentation_box.h"
#include "base/string_utilities.h"
#include "grt/common.h"
#include "grtpp_util.h"
#include <glibmm/main.h>
#include "workbench/wb_context_ui.h"
#include "gtk_helpers.h"

#define TIMER_INTERVAL 700

DocumentationBox::DocumentationBox() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), _multiple_items(false) {
  pack_start(_combo, false, false);

  Gtk::ScrolledWindow *swin = Gtk::manage(new Gtk::ScrolledWindow());
  swin->add(_text);
  swin->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  swin->set_shadow_type(Gtk::SHADOW_IN);

  _text.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  _text.signal_key_press_event().connect_notify(sigc::mem_fun(this, &DocumentationBox::text_key_press));
  _text.signal_button_press_event().connect_notify(sigc::mem_fun(this, &DocumentationBox::text_button_press));
  _text.get_buffer()->signal_changed().connect(sigc::mem_fun(this, &DocumentationBox::text_changed));

  Gtk::CellRendererText *cell = Gtk::manage(new Gtk::CellRendererText());
  _combo.pack_end(*cell, true);
  _combo.add_attribute(*cell, "text", 0);
  _combo.signal_changed().connect(sigc::mem_fun(this, &DocumentationBox::combo_changed));

  pack_start(*swin, true, true);

  show_all();
}

DocumentationBox::~DocumentationBox() {
  if (_timer)
    _timer.disconnect();
}

void DocumentationBox::update_for_form(bec::UIForm *form) {
  if (_timer)
    commit();

  _initializing = true;

  std::vector<std::string> items;
  grt::ListRef<GrtObject> new_object_list;
  std::string description;

  _selected_form = form;

  if (form)
    description = wb::WBContextUI::get()->get_description_for_selection(form, new_object_list, items);
  else
    description = wb::WBContextUI::get()->get_description_for_selection(new_object_list, items);

  // update only if selection was changed
  if (!grt::compare_list_contents(_object_list, new_object_list)) {

    // Set description text
    _object_list = new_object_list;

    // Set properties
    _multiple_items = items.size() > 1;


    // handle different number of selected items
    if (!items.empty()) {
      _combo.set_model(model_from_string_list(items, &_comboModel));

      _combo.set_active(0);

      // lock on multi selection
      if (_multiple_items) {
        _text.get_buffer()->set_text("<double-click to overwrite multiple objects>");
        _text.set_editable(false);
      } else {
        _text.get_buffer()->set_text(description);
        _text.set_editable(true);
      }
    } else {
      _combo.set_model(model_from_string_list({"No selection"}, &_comboModel));
      _combo.set_active(0);

      _text.get_buffer()->set_text("");
      _text.set_editable(false);
    }
    _combo.show_all();
  }

  _initializing = false;
}

void DocumentationBox::commit() {
  puts("COMMIT");
  _timer.disconnect();

  wb::WBContextUI::get()->set_description_for_selection(_object_list, _text.get_buffer()->get_text());
}

void DocumentationBox::text_changed() {
  if (!_initializing) {
    _timer.disconnect();
    _timer = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &DocumentationBox::commit), false),
                                            TIMER_INTERVAL);
  }
}

void DocumentationBox::text_button_press(GdkEventButton *ev) {
  if (ev->type == GDK_2BUTTON_PRESS && _multiple_items && !_text.get_editable()) {
    _initializing = true;

    _text.set_editable(true);
    _text.get_buffer()->set_text("");

    _initializing = false;
  }
}

void DocumentationBox::combo_changed() {
  if (!_initializing)
  {
    if (_combo.get_model()->children().size())
      _combo.set_active(0);
  }
}

void DocumentationBox::text_key_press(GdkEventKey *key) {
  if ((key->state & GDK_CONTROL_MASK) && key->keyval == GDK_KEY_Return && _text.get_editable()) {
    commit();
  }
}
