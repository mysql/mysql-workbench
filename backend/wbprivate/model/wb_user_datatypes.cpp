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

#include "workbench/wb_context.h"
#include "wb_context_model.h"
#include "wb_user_datatypes.h"
#include "mforms/menubar.h"

using namespace wb;
using namespace base;

UserDatatypeList::UserDatatypeList(WBContext *wb)
  : mforms::TreeView(mforms::TreeFlatList | mforms::TreeSidebar | mforms::TreeTranslucent), _wb(wb) {
  add_column(mforms::IconStringColumnType, "Type", 100);
  add_column(mforms::StringColumnType, "Definition", 80);
  add_column(mforms::StringColumnType, "Flags", 80);
  end_columns();

  _menu = new mforms::ContextMenu();
  _menu->add_item_with_title("Edit User Types...", std::bind(&UserDatatypeList::handle_menu_action, this, "edit"), "Edit User Types", "");
  set_context_menu(_menu);
}

UserDatatypeList::~UserDatatypeList() {
  delete _menu;
}

void UserDatatypeList::handle_menu_action(const std::string &action) {
  if (action == "edit")
    _wb->get_model_context()->show_user_type_editor(workbench_physical_ModelRef::cast_from(_catalog->owner()));
}

void UserDatatypeList::set_catalog(const db_CatalogRef &catalog) {
  _catalog = catalog;
}

static struct TypeIcon {
  const char *group;
  const char *icon;
} type_icons[] = {{"numeric", "db.DatatypeGroup.numeric.16x16.png"},
                  {"string", "db.DatatypeGroup.text.16x16.png"},
                  {"text", "db.DatatypeGroup.text.16x16.png"},
                  {"blob", "db.DatatypeGroup.blob.16x16.png"},
                  {"datetime", "db.DatatypeGroup.datetime.16x16.png"},
                  {"gis", "db.DatatypeGroup.geo.16x16.png"},
                  {"various", "db.DatatypeGroup.userdefined.16x16.png"},
                  {"userdefined", "db.DatatypeGroup.userdefined.16x16.png"},
                  {"structured", "db.DatatypeGroup.userdefined.16x16.png"},
                  {NULL, NULL}};

void UserDatatypeList::refresh() {
  clear();

  std::string deficon = bec::IconManager::get_instance()->get_icon_path("db.DatatypeGroup.userdefined.16x16.png");

  for (size_t c = _catalog->userDatatypes().count(), i = 0; i < c; i++) {
    db_UserDatatypeRef type(_catalog->userDatatypes()[i]);
    mforms::TreeNodeRef node = add_node();
    std::string icon = deficon;

    node->set_string(0, type->name());
    node->set_string(1, type->sqlDefinition());
    node->set_string(2, type->flags());

    if (type->actualType().is_valid() && type->actualType()->group().is_valid()) {
      const char *group = type->actualType()->group()->name().c_str();

      for (size_t i = 0; type_icons[i].group; i++) {
        if (strcmp(type_icons[i].group, group) == 0) {
          icon = bec::IconManager::get_instance()->get_icon_path(type_icons[i].icon);
          break;
        }
      }
    }
    node->set_icon_path(0, icon);
  }
}
