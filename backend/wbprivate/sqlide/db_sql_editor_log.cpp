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

#include "db_sql_editor_log.h"
#include "sqlide/recordset_data_storage.h"

#include "base/string_utilities.h"
#include "base/threading.h"
#include "base/wb_memory.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/log.h"

#include "mforms/utilities.h"
#include "wb_sql_editor_form.h"
#include "wb_sql_editor_panel.h"
#include <boost/foreach.hpp>

using namespace bec;
using namespace grt;
using namespace base;

// In the actions UI, truncate anything that's longer than this.
// In the actions log file we use the full string.
#define MAX_LOG_STATEMENT_TEXT 4098

//--------------------------------------------------------------------------------------------------

DbSqlEditorLog::DbSqlEditorLog(SqlEditorForm *owner, int max_entry_count)
  : VarGridModel(), _owner(owner), _max_entry_count(max_entry_count) {
  reset();
  _logDir = base::joinPath(bec::GRTManager::get()->get_user_datadir().c_str(), "log", "");
  create_directory(_logDir, 0700);

  _context_menu.add_item("Copy Row", "copy_row");
  _context_menu.add_item("Copy Action", "copy_action");
  _context_menu.add_item("Copy Response", "copy_message");
  _context_menu.add_item("Copy Duration", "copy_duration");
  _context_menu.add_separator();
  _context_menu.add_item("Append Selected Items to SQL script", "append_selected_items");
  _context_menu.add_item("Replace SQL Script With Selected Items", "replace_sql_script");
  _context_menu.add_separator();
  _context_menu.add_item("Clear", "clear");
  _context_menu.set_handler(std::bind(&DbSqlEditorLog::handle_context_menu, this, std::placeholders::_1));

  for (int i = 0; i < 8; i++)
    _context_menu.set_item_enabled(i, false);
}

//--------------------------------------------------------------------------------------------------

std::string DbSqlEditorLog::get_selection_text(bool time, bool query, bool result, bool duration) {
  std::string sql;
  for (std::vector<int>::const_iterator end = _selection.end(), it = _selection.begin(); it != end; ++it) {
    std::string s;
    bool need_tab = false;

    if (!sql.empty())
      sql.append("\n");

    if (time) {
      get_field(*it, 2, s);
      need_tab = true;
    }
    if (query) {
      if (need_tab)
        sql.append(s).append("\t");
      need_tab = true;
      get_field(*it, 3, s);
    }
    if (result) {
      if (need_tab)
        sql.append(s).append("\t");
      need_tab = true;
      get_field(*it, 4, s);
    }
    if (duration) {
      if (need_tab)
        sql.append(s).append("\t");
      get_field(*it, 5, s);
    }
    sql.append(s).append("\n");
  }
  return sql;
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorLog::handle_context_menu(const std::string &action) {
  std::string sql;
  if (action == "copy_row") {
    sql = get_selection_text(true, true, true, true);
    mforms::Utilities::set_clipboard_text(sql);
  } else if (action == "copy_action") {
    sql = get_selection_text(false, true, false, false);
    mforms::Utilities::set_clipboard_text(sql);
  } else if (action == "copy_message") {
    sql = get_selection_text(false, false, true, false);
    mforms::Utilities::set_clipboard_text(sql);
  } else if (action == "copy_duration") {
    sql = get_selection_text(false, false, false, true);
    mforms::Utilities::set_clipboard_text(sql);
  } else if (action == "append_selected_items") {
    sql = get_selection_text(false, true, false, false);
    SqlEditorPanel *editor(_owner->active_sql_editor_panel());
    if (editor)
      editor->editor_be()->append_text(sql);
  } else if (action == "replace_sql_script") {
    sql = get_selection_text(false, true, false, false);
    SqlEditorPanel *editor(_owner->active_sql_editor_panel());
    if (editor)
      editor->editor_be()->sql(sql.c_str());
  } else if (action == "clear") {
    reset();
  }
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorLog::set_selection(const std::vector<int> &selection) {
  _selection = selection;
  bool has_selection = !selection.empty();
  for (int i = 0; i < 8; i++)
    _context_menu.set_item_enabled(i, has_selection);
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorLog::reset() {
  VarGridModel::reset();

  {
    base::RecMutexLock data_mutex(_data_mutex);
    _data.clear();
    _next_id = 1;
  }

  _readonly = true;

  add_column("", int());  // msg type (icon)
  add_column("#", int()); // sequence no.
  add_column("Time", std::string());
  add_column("Action", std::string());
  add_column("Message", std::string());
  add_column("Duration / Fetch", std::string());

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  Recordset_data_storage::create_data_swap_tables(data_swap_db.get(), _column_names, _column_types);

  refresh_ui();
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorLog::refresh() {
  refresh_ui();
}

//--------------------------------------------------------------------------------------------------

class MsgTypeIcons {
public:

  MsgTypeIcons() {
    IconManager *icon_man = IconManager::get_instance();
    _error_icon = icon_man->get_icon_id("mini_error.png");
    _warning_icon = icon_man->get_icon_id("mini_warning.png");
    _info_icon = icon_man->get_icon_id("mini_notice.png");
    _ok_icon = icon_man->get_icon_id("mini_ok.png");
  }

private:
  IconId _error_icon;
  IconId _warning_icon;
  IconId _info_icon;
  IconId _ok_icon;

public:
  IconId icon(DbSqlEditorLog::MessageType msg_type) {
    switch (msg_type) {
      case DbSqlEditorLog::BusyMsg:
        return 0;
      case DbSqlEditorLog::NoteMsg:
        return _info_icon;
      case DbSqlEditorLog::OKMsg:
        return _ok_icon;
      case DbSqlEditorLog::ErrorMsg:
        return _error_icon;
      case DbSqlEditorLog::WarningMsg:
        return _warning_icon;
      default:
        return _info_icon;
    }
  }
};

//--------------------------------------------------------------------------------------------------

IconId DbSqlEditorLog::get_field_icon(const NodeId &node, ColumnId column, IconSize size) {
  IconId icon = 0;

  static MsgTypeIcons msg_type_icons;
  switch (column) {
    case 0:
      Cell cell;
      if (get_cell(cell, node, column, false)) {
        int msg_type = boost::get<int>(*cell);
        icon = msg_type_icons.icon((DbSqlEditorLog::MessageType)msg_type);
      }
      break;
  }

  return icon;
}

//--------------------------------------------------------------------------------------------------

static std::string sanitize_text(const std::string &text) {
  std::string output;
  for (std::string::const_iterator end = text.end(), ch = text.begin(); ch != end; ++ch) {
    if (*ch == '\n' || *ch == '\r' || *ch == '\t')
      output.push_back(' ');
    else
      output.push_back(*ch);
  }
  return output;
}

//--------------------------------------------------------------------------------------------------

bool DbSqlEditorLog::get_field(const bec::NodeId &node, ColumnId column, std::string &value) {
  if (VarGridModel::get_field(node, column, value)) {
    if (column == 3)
      value = sanitize_text(base::truncate_text(value, MAX_LOG_STATEMENT_TEXT));
    else if (column == 4)
      value = sanitize_text(value);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool DbSqlEditorLog::get_field_description_value(const bec::NodeId &node, ColumnId column, std::string &value) {
  return VarGridModel::get_field(node, column, value);
}

//--------------------------------------------------------------------------------------------------

RowId DbSqlEditorLog::add_message(int msg_type, const std::string &context, const std::string &msg,
                                  const std::string &duration) {
  if (msg.empty())
    return -1;

  std::string time = current_time();
  
  std::string logFileName = base::joinPath(_logDir.c_str(),
                                    sanitize_file_name("sql_actions_" + _owner->get_session_name() + ".log").c_str(), "");
  
  base::FILE_scope_ptr fp = base_fopen(logFileName.c_str(), "a");
  fprintf(fp, "[%u, %s] %s: %s\n", _next_id, time.c_str(), context.c_str(), msg.c_str());

  {
    base::RecMutexLock data_mutex(_data_mutex);

    // Remove oldest messages if there are more than the maximum allowed number.
    if (_max_entry_count > -1 && _max_entry_count - 1 < (int)_row_count) {
      _data.erase(_data.begin(), _data.begin() + (_row_count - _max_entry_count + 1) * _column_count);
      _row_count = _max_entry_count - 1;
    }

    add_message_with_id(_next_id, time, msg_type, context, msg, duration);
  }

  return _next_id++;
}

//--------------------------------------------------------------------------------------------------

void DbSqlEditorLog::set_message(RowId row, int msg_type, const std::string &context, const std::string &msg,
                                 const std::string &duration) {
  std::string time = current_time();
  {
    std::string logFileName = base::joinPath(_logDir.c_str(),
                                      sanitize_file_name("sql_actions_" + _owner->get_session_name() + ".log").c_str(), "");
    base::FILE_scope_ptr fp = base_fopen(logFileName.c_str(), "a");
    fprintf(fp, "[%u, %s] %s: %s\n", (unsigned)row, time.c_str(), context.c_str(), msg.c_str());
  }

  base::RecMutexLock data_mutex(_data_mutex);

  // Scan backwards for the given row id to find the right message entry.
  // Usually this will find the actual entry quickly, since normally add_message and set_message
  // aren't that much apart.
  // If the log has been cleared in the meantime however then simply add the message.
  if (_data.size() == 0) {
    add_message_with_id(row, time, msg_type, context, msg, duration);
    return;
  }

  Data::reverse_iterator cell = _data.rbegin() + _column_count - 2;
  while (cell != _data.rend()) {
    unsigned id = (unsigned)boost::apply_visitor(_var_to_int, *cell);
    if (id == row) {
      *(cell + 1) = msg_type;
      --cell;
      *--cell = base::strip_text(context);
      *--cell = msg;
      *--cell = duration;
      break;
    }
    cell += _column_count;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * This function does actually add the message and can also be called be set_message, if the
 * there's no message with a given id anymore.
 */
void DbSqlEditorLog::add_message_with_id(RowId id, const std::string &time, int msg_type, const std::string &context,
                                         const std::string &msg, const std::string &duration) {
  _data.reserve(_data.size() + _column_count);

  try {
    _data.push_back(msg_type);
    _data.push_back((int)id);
    _data.push_back(time);
    _data.push_back(base::strip_text(context));
    _data.push_back(msg);
    _data.push_back(duration);
  } catch (...) {
    _data.resize(_row_count * _column_count);
    throw;
  }

  ++_row_count;
  ++_data_frame_end;
}

//--------------------------------------------------------------------------------------------------

mforms::Menu *DbSqlEditorLog::get_context_menu() {
  return &_context_menu;
}

//--------------------------------------------------------------------------------------------------
