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

#include "active_label.h"
#include <mforms/app.h>
#include <gtkmm/stock.h>
#include <gtkmm/settings.h>

//--------------------------------------------------------------------------------
ActiveLabel::ActiveLabel(const Glib::ustring& text, const sigc::slot<void>& close_callback)
  : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL),
    _close_callback(close_callback),
    _text_label(text),
    _menu(NULL),
    _delete_menu(false) {
  set_spacing(5);

  if (!Gtk::Stock::lookup(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU, _closeImage)) {
    _closeImage.set(mforms::App::get()->get_resource_path("Close_16x16.png"));
    _closeImage.set_size_request(16, 16);
  }

  _btn_close.set_relief(Gtk::RELIEF_NONE);
  _btn_close.set_focus_on_click(false);
  _btn_close.add(_closeImage);
  _btn_close.add_events(Gdk::BUTTON_RELEASE_MASK);
  _btn_close.signal_button_release_event().connect(sigc::mem_fun(this, &ActiveLabel::handle_event), false);
  _btn_close.set_name("Close");
  _btn_close.get_style_context()->signal_changed().connect(sigc::mem_fun(this, &ActiveLabel::button_style_changed));

  _text_label_eventbox.set_visible_window(false);
  _text_label_eventbox.add(_text_label);

  pack_start(_text_label_eventbox);
  pack_start(_btn_close);

  show_all();

  pack_start(_spinner);
  _spinner.set_size_request(16, 16); //  Set the same size as the _closeImage, so the tab won't resize when swaping
  _spinner.hide();


  signal_button_press_event().connect(sigc::mem_fun(this, &ActiveLabel::button_press_slot));
}

//--------------------------------------------------------------------------------
ActiveLabel::~ActiveLabel() {
  if (_delete_menu)
    delete _menu;
}

//--------------------------------------------------------------------------------
bool ActiveLabel::handle_event(GdkEventButton* e) {
  switch (e->type) {
    case GDK_BUTTON_RELEASE: {
      if (e->button == 1 || e->button == 2) // left or middle mouse button
        _close_callback();
      break;
    }
    default:
      break;
  }

  return false;
}
//--------------------------------------------------------------------------------
void ActiveLabel::button_style_changed() {
  int w, h;
  if (Gtk::IconSize::lookup(Gtk::ICON_SIZE_MENU, w, h, _btn_close.get_settings()))
    _btn_close.set_size_request(w - 2, h - 2);
}
//--------------------------------------------------------------------------------
void ActiveLabel::set_menu(mforms::Menu* m, bool delete_when_done) {
  this->_menu = m;
  _delete_menu = delete_when_done;
}

bool ActiveLabel::has_menu() {
  return this->_menu != NULL;
}

//--------------------------------------------------------------------------------
void ActiveLabel::set_text(const std::string& lbl) {
  _text_label.set_text(lbl);
}

//--------------------------------------------------------------------------------
bool ActiveLabel::button_press_slot(GdkEventButton* evb) {
  if (evb->button == 3 && _menu && !_menu->empty())
    _menu->popup_at(0, evb->x, evb->y);
  else if (evb->button == 2 && _close_callback) // middle button
    _close_callback();
  return false;
}

//--------------------------------------------------------------------------------
void ActiveLabel::start_busy() {
  _btn_close.hide();

  _spinner.show();
  _spinner.start();
}

//--------------------------------------------------------------------------------
void ActiveLabel::stop_busy() {
  _spinner.stop();
  _spinner.hide();

  _btn_close.show();
}
