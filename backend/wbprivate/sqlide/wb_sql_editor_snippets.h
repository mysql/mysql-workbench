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

#ifndef _DB_SQL_EDITOR_SNIPPETSBE_H_
#define _DB_SQL_EDITOR_SNIPPETSBE_H_

#include "workbench/wb_backend_public_interface.h"
#include "base/ui_form.h"
#include "grt/tree_model.h"

#include <deque>

namespace wb {
  class WBContextSQLIDE;
};
class SqlEditorForm;

#define USER_SNIPPETS "My Snippets"
#define SHARED_SNIPPETS "Shared"

class MYSQLWBBACKEND_PUBLIC_FUNC DbSqlEditorSnippets : public bec::ListModel {
public:
  enum Column { Description, Script };

  static void setup(wb::WBContextSQLIDE *sqlide, const std::string &path);
  static DbSqlEditorSnippets *get_instance();

  void load();
  void save();
  void load_from_db(SqlEditorForm *editor = 0);

  bool shared_snippets_usable();

  std::vector<std::string> get_category_list();
  void select_category(const std::string &category);
  std::string selected_category();

  virtual size_t count();
  virtual bool get_field(const bec::NodeId &node, ColumnId column, std::string &value);
  virtual bool set_field(const bec::NodeId &node, ColumnId column, const std::string &value);
  virtual void refresh() {
  }

  // virtual bool activate_node(const bec::NodeId &node);

  bool activate_toolbar_item(const bec::NodeId &selected, const std::string &name);

  //  virtual bec::MenuItemList get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes);
  //  virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &nodes);

  virtual bool can_delete_node(const bec::NodeId &node);
  virtual bool delete_node(const bec::NodeId &node);

protected:
  DbSqlEditorSnippets(wb::WBContextSQLIDE *sqlide, const std::string &path);
  wb::WBContextSQLIDE *_sqlide;
  std::string _path;
  std::string _snippet_db;
  std::string _selected_category;
  bool _shared_snippets_enabled;

  struct Snippet {
    std::string title;
    std::string code;
    int db_snippet_id; // only if it comes from the DB
  };

  std::deque<Snippet> _entries;

  void toolbar_item_activated(const std::string &name);
  void copy_original_file(const std::string &name, bool overwrite);

  int add_db_snippet(const std::string &name, const std::string &code);
  void delete_db_snippet(int snippet_id);

public:
  void add_snippet(const std::string &name, const std::string &code, bool edit);
};

#endif /* _DB_SQL_EDITOR_SNIPPETSBE_H_ */
