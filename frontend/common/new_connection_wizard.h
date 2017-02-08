/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
