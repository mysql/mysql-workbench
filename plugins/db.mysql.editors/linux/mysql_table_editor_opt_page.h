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

#ifndef __WB_MYSQL_TABLE_EDITOR_OPT_PAGE_H__
#define __WB_MYSQL_TABLE_EDITOR_OPT_PAGE_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>

class MySQLTableEditorBE;
class DbMySQLTableEditor;

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorOptPage {
public:
  DbMySQLTableEditorOptPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be, Glib::RefPtr<Gtk::Builder> xml);

  void refresh();

  void switch_be(MySQLTableEditorBE* be);

private:
  void set_table_option(const std::string& value, const char* option);
  void set_toggled_table_option(const char* option);

  void set_pack_keys();
  void set_row_format();
  void set_key_block_size();
  void set_merge_method();

  DbMySQLTableEditor* _owner;
  MySQLTableEditorBE* _be;
  Glib::RefPtr<Gtk::Builder> _xml;

  bool _refreshing;
};

#endif
