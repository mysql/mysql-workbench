/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtdb_connect_dialog.h"

#include "base/string_utilities.h"

#include "mforms/uistyle.h"
#include "mforms/utilities.h"

using namespace grtui;

DbConnectionDialog::DbConnectionDialog(const db_mgmt_ManagementRef &mgmt)
  : mforms::Form(0), _panel(true), _top_vbox(false), _bottom_hbox(true) {
  set_content(&_top_vbox);
  set_name("Connection");
  setInternalName("connection_dialog");

  _panel.init(mgmt);

  _top_vbox.set_padding(MF_WINDOW_PADDING);

  _top_vbox.add(&_panel, true, true);
  _top_vbox.add(&_bottom_hbox, false, true);

  mforms::Utilities::add_end_ok_cancel_buttons(&_bottom_hbox, &_ok_button, &_cancel_button);

  _bottom_hbox.set_spacing(MF_BUTTON_SPACING);
  _ok_button.set_text(_("OK"));
  _cancel_button.set_text(_("Cancel"));

  set_title(_("Connect to Database"));

  scoped_connect(_ok_button.signal_clicked(), std::bind(&DbConnectionDialog::ok_clicked, this));
  scoped_connect(_cancel_button.signal_clicked(), std::bind(&DbConnectionDialog::cancel_clicked, this));

  set_size(700, 500);
  center();
}

db_mgmt_ConnectionRef DbConnectionDialog::run() {
  if (run_modal(&_ok_button, &_cancel_button))
    return _panel.get_connection();

  return db_mgmt_ConnectionRef();
}

void DbConnectionDialog::ok_clicked() {
}

void DbConnectionDialog::cancel_clicked() {
}

void DbConnectionDialog::test_clicked() {
}
