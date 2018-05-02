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

#ifndef _DB_SQL_EDITOR_LOG_BE_H_
#define _DB_SQL_EDITOR_LOG_BE_H_

#include "workbench/wb_backend_public_interface.h"
#include "sqlide/var_grid_model_be.h"
#include "mforms/menu.h"

class SqlEditorForm;

class MYSQLWBBACKEND_PUBLIC_FUNC DbSqlEditorLog : public VarGridModel {
public:
  enum MessageType { ErrorMsg, WarningMsg, NoteMsg, OKMsg, BusyMsg };

  typedef std::shared_ptr<DbSqlEditorLog> Ref;

  virtual ~DbSqlEditorLog() {
  }

  static Ref create(SqlEditorForm *owner, int max_entry_count) {
    return Ref(new DbSqlEditorLog(owner, max_entry_count));
  }

  virtual void reset();
  virtual void refresh();

  virtual bec::IconId get_field_icon(const bec::NodeId &node, ColumnId column, bec::IconSize size);
  virtual bool get_field(const bec::NodeId &node, ColumnId column, std::string &value);
  virtual bool get_field(const bec::NodeId &node, ColumnId column, ssize_t &value) {
    return VarGridModel::get_field(node, column, value);
  }
  virtual bool get_field_description_value(const bec::NodeId &node, ColumnId column, std::string &value);

  std::string get_selection_text(bool time, bool query, bool result, bool duration);

  RowId add_message(int msg_type, const std::string &context, const std::string &msg, const std::string &duration);
  void set_message(RowId row, int msg_type, const std::string &context, const std::string &msg,
                   const std::string &duration);

  mforms::Menu *get_context_menu();
  void set_selection(const std::vector<int> &selection);

protected:
  // max_entry_count < 0 means unlimited number of messages.
  DbSqlEditorLog(SqlEditorForm *owner, int max_entry_count);

  void add_message_with_id(RowId id, const std::string &time, int msg_type, const std::string &context,
                           const std::string &msg, const std::string &duration);

private:
  SqlEditorForm *_owner;
  mforms::Menu _context_menu;
  std::vector<int> _selection;
  int _max_entry_count;       // For the internal list which is used in the UI.
  std::string _logDir;
  unsigned _next_id;

  void handle_context_menu(const std::string &action);
};

#endif /* _DB_SQL_EDITOR_LOG_BE_H_ */
