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

#ifndef __ACTIVE_LABEL_H__
#define __ACTIVE_LABEL_H__

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/button.h>
#include <gtkmm/spinner.h>
#include "mforms/menu.h"

#include <sigc++/sigc++.h>

//==============================================================================
//
//==============================================================================
class ActiveLabel : public Gtk::Box {
public:
  ActiveLabel(const Glib::ustring& text, const sigc::slot<void>& close_callback);
  virtual ~ActiveLabel();

  void set_text(const std::string& lbl);
  std::string get_text() const {
    return _text_label.get_text();
  }

  mforms::Menu* get_menu() {
    return _menu;
  }
  void set_menu(mforms::Menu* m, bool delete_when_done);
  bool has_menu();
  void start_busy();
  void stop_busy();

  void call_close() {
    _close_callback();
  }

private:
  bool button_press_slot(GdkEventButton*);
  bool handle_event(GdkEventButton*);
  void button_style_changed();
  const sigc::slot<void> _close_callback;
  Gtk::Button _btn_close;
  Gtk::Image _closeImage;
  Gtk::EventBox _text_label_eventbox;
  Gtk::Label _text_label;
  mforms::Menu* _menu;
  Gtk::Spinner _spinner;
  bool _delete_menu;
};

#endif
