/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _NEWCONNECTIONWIZARD_H_
#define _NEWCONNECTIONWIZARD_H_

#include "workbench/wb_backend_public_interface.h"

#include "grtui/db_conn_be.h"
#include "grtui/grtdb_connect_panel.h"
#include "grt/grt_manager.h"

#include "mforms/form.h"
#include "mforms/uistyle.h"
#include "mforms/button.h"
#include "mforms/view.h"

namespace wb {
  class WBContext;
};

class MYSQLWBBACKEND_PUBLIC_FUNC NewConnectionWizard : public mforms::Form {
public:
  NewConnectionWizard(wb::WBContext *context, const db_mgmt_ManagementRef &mgmt);
  ~NewConnectionWizard();

  db_mgmt_ConnectionRef run();

private:
  wb::WBContext *_context;
  db_mgmt_ManagementRef _mgmt;
  grtui::DbConnectPanel _panel;
  db_mgmt_ConnectionRef _connection;
  mforms::Box _top_vbox;

  mforms::TextEntry *_conn_name;

  mforms::Box _bottom_hbox;
  mforms::Button _ok_button;
  mforms::Button _cancel_button;
  mforms::Button _test_button;
  mforms::Button _config_button;

  void open_remote_mgm_config();
};

#endif /* _NEWCONNECTIONWIZARD_H_ */
