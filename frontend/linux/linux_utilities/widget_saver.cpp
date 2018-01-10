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

#include "base/log.h"
#include "widget_saver.h"
#include <gtkmm/paned.h>
#include <glibmm/main.h>
#include "grt/grt_manager.h"
#include "mforms/toolbar.h"

DEFAULT_LOG_DOMAIN("gtk.utils")

//------------------------------------------------------------------------------
void utils::gtk::save_settings(Gtk::Paned* paned, const bool right_side) {
  const std::string name = paned->get_name();
  if (!name.empty() && paned->get_data("allow_save")) {
    long pos = paned->get_position();
    if (right_side)
      pos = paned->get_width() - pos;
    bec::GRTManager::get()->set_app_option(name + ".position", grt::IntegerRef(pos));
  }
}

//------------------------------------------------------------------------------
static bool set_paned_position(Gtk::Paned* paned, const long pos, const bool right, const int min_size) {
  if (right) {
    int size;
    if (paned->get_orientation() == Gtk::ORIENTATION_HORIZONTAL)
      size = paned->get_width() - pos;
    else
      size = paned->get_height() - pos;
    if (min_size > 0 && size < min_size)
      size = min_size;
    else if (min_size < 0 && size > abs(min_size))
      size = abs(min_size);
    paned->set_position(size);
  } else {
    int npos = pos;
    if (min_size > 0 && npos < min_size)
      npos = min_size;
    else if (min_size < 0 && npos > abs(min_size))
      npos = abs(min_size);
    paned->set_position(npos);
  }
  paned->set_data("allow_save", (void*)1);

  return false;
}

//------------------------------------------------------------------------------
sigc::connection utils::gtk::load_settings(Gtk::Paned* paned, const sigc::slot<void> defaults_slot,
                                           const bool right_side, const int min_size) {
  const std::string name = paned->get_name();
  long pos = -1;
  try {
    pos = bec::GRTManager::get()->get_app_option_int(name + ".position");
  } catch (const grt::type_error& e) {
    logError("Can not restore paned position for name '%s', error '%s'\n", name.c_str(), e.what());
  }
  sigc::connection con;
  if (pos > 0) {
    paned->set_data("allow_save", NULL);
    con = Glib::signal_idle().connect(sigc::bind(sigc::ptr_fun(&set_paned_position), paned, pos, right_side, min_size));
  } else {
    defaults_slot();
    paned->set_data("allow_save", (void*)1);
  }
  return con;
}
