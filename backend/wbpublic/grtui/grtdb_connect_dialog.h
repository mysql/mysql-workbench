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

#ifndef _GRTDBCONNECTIONDIALOG_H_
#define _GRTDBCONNECTIONDIALOG_H_

#include <mforms/form.h>
#include <mforms/box.h>
#include <mforms/button.h>

#include "db_conn_be.h"
#include "grtdb_connect_panel.h"
#include "grt/grt_manager.h"

namespace grtui {

  class WBPUBLICBACKEND_PUBLIC_FUNC DbConnectionDialog : public mforms::Form {
  public:
    DbConnectionDialog(const db_mgmt_ManagementRef &mgmt);

    db_mgmt_ConnectionRef run();

  protected:
    db_mgmt_ManagementRef _mgmt;
    DbConnectPanel _panel;

    mforms::Box _top_vbox;

    mforms::Box _bottom_hbox;
    mforms::Button _ok_button;
    mforms::Button _cancel_button;
    mforms::Button _test_button;

    //! Called from selector/list of connections to tell that things changed
    void change_active_stored_conn();

  private:
    void ok_clicked();
    void cancel_clicked();
    void test_clicked();

    void reset_stored_conn_list();
  };
};

#endif /* _GRTDBCONNECTIONDIALOG_H_ */
