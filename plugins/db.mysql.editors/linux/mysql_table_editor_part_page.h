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

#ifndef __WB_MYSQL_TABLE_EDITOR_PART_PAGE_H__
#define __WB_MYSQL_TABLE_EDITOR_PART_PAGE_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>
#include <gtkmm/combobox.h>
#include <gtkmm/togglebutton.h>

class MySQLTableEditorBE;
class ListModelWrapper;
class DbMySQLTableEditor;

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorPartPage {
public:
  DbMySQLTableEditorPartPage(DbMySQLTableEditor *owner, MySQLTableEditorBE *be, Glib::RefPtr<Gtk::Builder> xml);

  void refresh();

  void switch_be(MySQLTableEditorBE *be);

private:
  void init_widgets();
  void enabled_checkbutton_toggled();

  void part_function_changed();
  void subpart_function_changed();

  void part_count_changed();
  void subpart_count_changed();

  void part_manual_toggled();
  void subpart_manual_toggled();

  void set_part_params_to_be(const std::string &value);
  void set_subpart_params_to_be(const std::string &value);

  DbMySQLTableEditor *_owner;
  MySQLTableEditorBE *_be;
  Glib::RefPtr<Gtk::Builder> _xml;

  Gtk::ComboBox *_part_by_combo;
  Gtk::ComboBox *_subpart_by_combo;
  Gtk::Entry *_part_count_entry;
  Gtk::Entry *_subpart_count_entry;
  Gtk::Entry *_part_params_entry;
  Gtk::Entry *_subpart_params_entry;
  Gtk::ToggleButton *_part_manual_checkbtn;
  Gtk::ToggleButton *_subpart_manual_checkbtn;

  Gtk::TreeView *_part_tv;
  Glib::RefPtr<ListModelWrapper> _part_model;

  bool _refreshing;
};

#endif
