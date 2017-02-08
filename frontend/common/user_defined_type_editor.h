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

#pragma once

#include "workbench/wb_backend_public_interface.h"
#include "grts/structs.workbench.physical.h"

#include "mforms/treeview.h"
#include "mforms/table.h"
#include "mforms/box.h"
#include "mforms/label.h"
#include "mforms/textentry.h"
#include "mforms/selector.h"
#include "mforms/button.h"
#include "mforms/form.h"

namespace wb {
  class WBContextUI;
};

namespace mforms {
  class CheckBox;
};

class MYSQLWBBACKEND_PUBLIC_FUNC UserDefinedTypeEditor : public mforms::Form {
  workbench_physical_ModelRef _model;

  mforms::Box _vbox;
  mforms::TreeView _type_list;

  mforms::Table _table;
  mforms::Label _namel;
  mforms::TextEntry _name;
  mforms::Label _typel;
  mforms::Selector _type;
  mforms::Label _argsl;
  mforms::Box _args_box;
  mforms::TextEntry _args;
  mforms::Button _args_edit;
  mforms::Label _flagsl;
  mforms::Box _flags_box;

  mforms::Box _button_box;
  mforms::Button _add_button;
  mforms::Button _delete_button;

  mforms::Button _ok_button;
  mforms::Button _cancel_button;

  std::vector<mforms::CheckBox *> _flags;
  std::vector<db_UserDatatypeRef> _user_types;

  std::vector<db_SimpleDatatypeRef> _valid_types;

  bool is_type_used(const db_UserDatatypeRef &type);

  void add_clicked();
  void delete_clicked();
  void ok_clicked();
  void cancel_clicked();
  void edit_arguments();

  void name_changed();
  void args_changed();
  void type_changed();
  void flag_toggled();
  void refresh();
  void selected_row();

public:
  UserDefinedTypeEditor(const workbench_physical_ModelRef &model);
};
