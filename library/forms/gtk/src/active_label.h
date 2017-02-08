/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef __ACTIVE_LABEL_H__
#define __ACTIVE_LABEL_H__

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/button.h>
#include "mforms/menu.h"
#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20
#include <gtkmm/spinner.h>
#endif

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
#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20
  Gtk::Spinner _spinner;
#endif
  bool _delete_menu;
};

#endif
