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


#include "mforms/mforms.h"

#include "gtk/lf_mforms.h"

#include <gtkmm.h>

using namespace mforms;

int main(int argc, char **argv) {
  Gtk::Main main(argc, argv);

  ::mforms::gtk::init();

  Form window;
  Box hbox(true);

  window.set_size(400, 400);

  window.set_content(&hbox);

  Button b1;
  Button b2;
  Button b3;

  b1.set_text("Button1");
  b2.set_text("Button2");
  b3.set_text("Button3");

  hbox.add(&b1, true, true);
  hbox.add(&b2, true, false);
  hbox.add(&b3, false, false);

  hbox.set_spacing(8);
  hbox.set_padding(12);

  window.show();

  main.run();
  return 0;
}
