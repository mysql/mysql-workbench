/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
