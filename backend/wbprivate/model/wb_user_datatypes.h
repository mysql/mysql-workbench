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

#ifndef _WB_USER_DATATYPES_H_
#define _WB_USER_DATATYPES_H_

#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"

#include <grtpp_undo_manager.h>

#include "mforms/treeview.h"

#include "workbench/wb_backend_public_interface.h"

namespace wb {
  class WBContext;

  class MYSQLWBBACKEND_PUBLIC_FUNC UserDatatypeList : public mforms::TreeView {
  public:
    UserDatatypeList(WBContext *wb);
    virtual ~UserDatatypeList();

    void set_catalog(const db_CatalogRef &catalog);

    void refresh();

  protected:
    mforms::ContextMenu *_menu;
    db_CatalogRef _catalog;
    WBContext *_wb;

    void handle_menu_action(const std::string &action);
  };
};

#endif
