/*
 * Copyright (c) 2009, 2023, Oracle and/or its affiliates. All rights reserved.
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

#include <fstream>
#include <glib/gstdio.h>
#include <stdlib.h>

#include <boost/foreach.hpp>

#include "db_sql_editor_history_be.h"
#include "sqlide/recordset_data_storage.h"

#include "base/threading.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/scope_exit_trigger.h"
#include "base/xml_functions.h"

#include "mforms/utilities.h"

DEFAULT_LOG_DOMAIN("sqlide-history")

using namespace bec;
using namespace grt;
using namespace base;

const char *SQL_HISTORY_DIR_NAME = "sql_history";

DbSqlEditorHistory::DbSqlEditorHistory() : _current_entry_index(-1) {
  _entries_model = EntriesModel::create(this);
  _details_model = DetailsModel::create();
  _write_only_details_model = DetailsModel::create();
  load();
}

DbSqlEditorHistory::~DbSqlEditorHistory() {
}

void DbSqlEditorHistory::reset() {
  _details_model->reset();
  _entries_model->reset();
  _current_entry_index = -1;
}

void DbSqlEditorHistory::load() {
  _entries_model->load();
}

void DbSqlEditorHistory::add_entry(const std::list<std::string> &statements) {
  size_t old_date_count = _details_model->count();
  _entries_model->add_statements(statements);

  if (_entries_model->get_ui_usage()) {
    _entries_model->refresh_ui();
    if (old_date_count < _details_model->count())
      _details_model->refresh_ui();
  }
}

void DbSqlEditorHistory::current_entry(int index) {
  if (index < 0)
    _details_model->reset();
  else {
    update_timestamp(_entries_model->entry_date(index));
    _details_model->load(_entries_model->entry_path(index));
  }

  _current_entry_index = index;

  // Indicates to the entries model if its changes affect UI or not
  // UI is affected only in the first entry ( where the new statements are appended )
  _entries_model->set_ui_usage(_current_entry_index == 0);
  _entries_model->refresh();
  _details_model->refresh();
}

std::string DbSqlEditorHistory::restore_sql_from_history(int entry_index, std::list<int> &detail_indexes) {
  std::string sql;
  if (entry_index >= 0) {
    DetailsModel::Ref details_model;
    if (entry_index == _current_entry_index)
      details_model = _details_model;
    else {
      details_model = DetailsModel::create();
      details_model->load(_entries_model->entry_path(entry_index));
    }
    std::string statement;
    for (int row : detail_indexes) {
      details_model->get_field(row, 1, statement);
      sql += statement + ";\n";
    }
  }
  return sql;
}

//------------------------------

DbSqlEditorHistory::EntriesModel::EntriesModel(DbSqlEditorHistory *owner)
  : VarGridModel(), _ui_usage(false), _owner(owner) {
  reset();
}

void DbSqlEditorHistory::EntriesModel::reset() {
  VarGridModel::reset();

  _readonly = true;
  add_column("Date", std::string());

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  Recordset_data_storage::create_data_swap_tables(data_swap_db.get(), _column_names, _column_types);

  refresh_ui();
}

void DbSqlEditorHistory::EntriesModel::load() {
  std::string sql_history_dir = base::makePath(bec::GRTManager::get()->get_user_datadir(), SQL_HISTORY_DIR_NAME);
  g_mkdir_with_parents(sql_history_dir.c_str(), 0700);
  {
    GError *error = NULL;
    GDir *dir = g_dir_open(sql_history_dir.c_str(), 0, &error);
    if (!dir) {
      grt::GRT::get()->send_error(_("Can't open SQL history directory"),
                                  (error ? error->message : sql_history_dir.c_str()));
      return;
    }
    // files are not read in alpha-order, so we need to sort them before inserting
    std::set<std::string> entries;
    base::ScopeExitTrigger on_scope_exit(std::bind(&g_dir_close, dir));
    while (const char *name_ = g_dir_read_name(dir)) {
      // file name is expected in "YYYY-MM-DD" format
      std::string name(name_);
      if (name.size() != 10)
        continue;
      name[4] = '\0';
      name[7] = '\0';
      entries.insert(name);
    }

    for (std::set<std::string>::const_iterator name = entries.begin(); name != entries.end(); ++name) {
      tm t;
      memset(&t, 0, sizeof(t));
      t.tm_year = base::atoi<int>(&(*name)[0], 0) - 1900;
      t.tm_mon = base::atoi<int>(&(*name)[5], 0) - 1;
      t.tm_mday = base::atoi<int>(&(*name)[8], 0);
      if (t.tm_year != 0)
        insert_entry(t);
    }
  }
}

void DbSqlEditorHistory::EntriesModel::add_statements(const std::list<std::string> &statements) {
  if (statements.empty())
    return;

  std::tm timestamp = local_timestamp();
  bool new_date = insert_entry(timestamp);

  std::string time = format_time(timestamp, "%X");
  std::list<std::string> timed_statements;

  for (std::string statement : statements) {
    timed_statements.push_back(time);
    timed_statements.push_back(base::strip_text(statement));
  }

  if (new_date) {
    refresh_ui();
    _owner->current_entry((int)_row_count - 1);
    _owner->update_timestamp(timestamp);
  }

  if (_ui_usage)
    _owner->details_model()->add_entries(timed_statements);
  else
    _owner->write_only_details_model()->add_entries(timed_statements);
}

bool DbSqlEditorHistory::EntriesModel::insert_entry(const std::tm &t) {
  std::string newest_date;
  if (_row_count > 0)
    get_field(NodeId(0), 0, newest_date);
  std::string date = format_time(t, "%Y-%m-%d");
  if (date != newest_date) {
    base::RecMutexLock data_mutex(_data_mutex);
    _data.insert(_data.begin(), date);
    ++_row_count;
    ++_data_frame_end;
    return true;
  }
  return false;
}

bec::MenuItemList DbSqlEditorHistory::EntriesModel::get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes) {
  bec::MenuItemList items;
  bec::MenuItem item;

  item.internalName = "delete_selection";
  item.accessibilityName = "Delete Selection";
  item.caption = "Delete Selected Date Log";
  item.enabled = nodes.size() > 0;
  items.push_back(item);

  item.internalName = "delete_all";
  item.accessibilityName = "Delete All";
  item.caption = "Delete All Logs";
  item.enabled = true;
  items.push_back(item);

  return items;
}

bool DbSqlEditorHistory::EntriesModel::activate_popup_item_for_nodes(const std::string &action,
                                                                     const std::vector<bec::NodeId> &orig_nodes) {
  if (action == "delete_selection") {
    std::vector<std::size_t> rows;
    rows.reserve(orig_nodes.size());
    for (const bec::NodeId &node : orig_nodes)
      rows.push_back(node[0]);
    delete_entries(rows);

    return true;
  } else if (action == "delete_all") {
    delete_all_entries();
    return true;
  }

  return false;
}

void DbSqlEditorHistory::EntriesModel::delete_all_entries() {
  if (mforms::Utilities::show_message(
        "Clear History", "Do you really want to delete the entire query history?\nThis operation cannot be undone.",
        "Delete All", "Cancel", "") == mforms::ResultCancel)
    return;

  std::vector<std::size_t> rows;
  rows.reserve(_row_count);
  for (RowId row = 0; row < _row_count; ++row)
    rows.push_back(row);
  delete_entries(rows);
}

void DbSqlEditorHistory::EntriesModel::delete_entries(const std::vector<std::size_t> &rows) {
  if (rows.empty())
    return;
  {
    std::vector<size_t> sorted_rows = rows;
    std::sort(sorted_rows.begin(), sorted_rows.end());
    BOOST_REVERSE_FOREACH(size_t row, sorted_rows) {
      try {
        base::remove(entry_path(row));
      } catch (const std::exception &exc) {
        logError("Error deleting log entry %s: %s\n", entry_path(row).c_str(), exc.what());
      }
      Cell row_begin = _data.begin() + row * _column_count;
      _data.erase(row_begin, row_begin + _column_count);
      --_row_count;
    }
  }
  refresh_ui();
  _owner->current_entry(-1);
}

std::string DbSqlEditorHistory::EntriesModel::entry_path(std::size_t index) {
  std::string name;
  get_field(index, 0, name);
  std::string storage_file_path = base::makePath(bec::GRTManager::get()->get_user_datadir(), SQL_HISTORY_DIR_NAME);
  storage_file_path = base::makePath(storage_file_path, name);
  return storage_file_path;
}

std::tm DbSqlEditorHistory::EntriesModel::entry_date(std::size_t index) {
  tm t;
  std::string name;
  get_field(index, 0, name);

  memset(&t, 0, sizeof(t));
  t.tm_year = base::atoi<int>(&name[0], 0) - 1900;
  t.tm_mon = base::atoi<int>(&name[5], 0) - 1;
  t.tm_mday = base::atoi<int>(&name[8], 0);

  return t;
}

//--------------------------------------------------------------------------------------------------
DbSqlEditorHistory::DetailsModel::DetailsModel() : VarGridModel() {
  reset();

  _context_menu.add_item(_("Copy Row To Clipboard"), "copy_row");
  _context_menu.add_separator();
  _context_menu.add_item(_("Append Selected Items to SQL script"), "append_selected_items");
  _context_menu.add_item(_("Replace SQL Script With Selected Items"), "replace_sql_script");
}

void DbSqlEditorHistory::DetailsModel::reset() {
  VarGridModel::reset();

  _last_loaded_row = -1;
  _last_timestamp = std::string("");
  _last_statement = std::string("");
  _datestamp = local_timestamp();

  _readonly = true;

  add_column("Time", std::string());
  add_column("SQL", std::string());

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  Recordset_data_storage::create_data_swap_tables(data_swap_db.get(), _column_names, _column_types);

  refresh_ui();
}

void DbSqlEditorHistory::DetailsModel::load(const std::string &storage_file_path) {
  if (base::file_exists(storage_file_path)) {
    std::ifstream historyXml(storage_file_path);
    if (historyXml.is_open()) {
      std::string line;

      // Skips the first line in the file as is the xml header
      std::getline(historyXml, line);
      _row_count = 0;

      base::RecMutexLock data_mutex(_data_mutex);
      _data.clear();
      _data.reserve(_data.size() + _column_count);
      while (historyXml.good()) {
        std::getline(historyXml, line);

        if (line.empty())
          continue;

        xmlDocPtr xmlDoc = base::xml::xmlParseFragment(line);
        if (xmlDoc == nullptr) {
          logError("Can't parse %s, of file: %s\n", line.c_str(), storage_file_path.c_str());
          continue;
        }

        // In history we've got one element per line.
        auto element = xmlDoc->children;
        if (element->next !=
            nullptr) // If there's something more, we log proper information and parse only that one element.
          logError("History line contains too many elements %s, of file: %s\n", line.c_str(),
                   storage_file_path.c_str());

        std::string timestamp = base::xml::getProp(element, "timestamp");
        std::string statement = base::xml::getContent(element);
        // decides whether to use or not the existing data
        if (timestamp != _last_timestamp.toString() && timestamp != "~")
          _last_timestamp = timestamp;

        if (statement != _last_statement.toString() && statement != "~")
          _last_statement = statement;

        _data.push_back(_last_statement);
        _data.push_back(_last_timestamp);

        _row_count++;

        xmlFree(xmlDoc);
      }

      std::reverse(_data.begin(), _data.end());

      _data_frame_end = _row_count;

      _last_loaded_row = (int)_row_count - 1;
    }
  } else
    logError("Can't open SQL history file %s\n", storage_file_path.c_str());
}

std::string DbSqlEditorHistory::DetailsModel::storage_file_path() const {
  std::string storage_file_path = base::makePath(bec::GRTManager::get()->get_user_datadir(), SQL_HISTORY_DIR_NAME);
  storage_file_path = base::makePath(storage_file_path, format_time(_datestamp, "%Y-%m-%d"));
  return storage_file_path;
}

void DbSqlEditorHistory::DetailsModel::save() {
  std::string storage_file_path = this->storage_file_path();
  std::ofstream ofs;
  {
    std::string storage_file_dir = base::makePath(bec::GRTManager::get()->get_user_datadir(), SQL_HISTORY_DIR_NAME);
    if (g_mkdir_with_parents(storage_file_dir.c_str(), 0700) != -1) {
      bool is_file_new = (g_file_test(storage_file_path.c_str(), G_FILE_TEST_EXISTS) == 0);
      if (is_file_new || g_file_test(storage_file_path.c_str(), G_FILE_TEST_IS_REGULAR)) {
        ofs.open(base::path_from_utf8(storage_file_path).c_str(), std::ios_base::app);
        if (is_file_new)
          ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
      }
    }
  }

  if (!ofs.is_open() || !ofs) {
    grt::GRT::get()->send_error("Can't write to SQL history file", storage_file_path);
    return;
  }

  {

    base::RecMutexLock data_mutex(_data_mutex);
    std::reverse(_data.begin(), _data.end());
    std::string last_saved_timestamp;
    std::string last_saved_statement;
    get_field(NodeId(_last_loaded_row), 1, last_saved_timestamp);
    get_field(NodeId(_last_loaded_row), 0, last_saved_statement);

    for (RowId row = _last_loaded_row + 1; row < _row_count; ++row) {
      std::string time, sql;
      get_field(NodeId((int)row), 1, time);
      get_field(NodeId((int)row), 0, sql);

      if (time == last_saved_timestamp)
        time = "~";
      else
        last_saved_timestamp = time;

      if (sql == last_saved_statement)
        sql = "~";
      else
        last_saved_statement = sql;

      std::string xml_time, xml_sql;
      xml_time = base::xml::encodeEntities(time);
      xml_sql = base::xml::encodeEntities(sql);
      xml_sql = base::replaceString(xml_sql, "\n", "&#x0A;");
      ofs << "<ENTRY timestamp=\'" << xml_time << "\'>" << xml_sql << "</ENTRY>\n";
    }
    _last_loaded_row = (int)_row_count - 1;
    std::reverse(_data.begin(), _data.end());
  }
  ofs.flush();
}

void DbSqlEditorHistory::DetailsModel::add_entries(const std::list<std::string> &statements) {
  if (statements.empty())
    return;

  {
    base::RecMutexLock data_mutex(_data_mutex);

    _data.reserve(_data.size() + _column_count);

    try {
      int index = 0;
      for (auto rit = statements.rbegin(); rit != statements.rend(); rit++) {
        if (index % 2) {
          // decides whether to use or not the existing data
          if (*rit != _last_timestamp.toString())
            _last_timestamp = *rit;

          _data.insert(_data.begin(), _last_timestamp);
        } else {
          if (*rit != _last_statement.toString())
            _last_statement = *rit;

          _data.insert(_data.begin(), _last_statement);
        }
        index++;
      }

    } catch (...) {
      _data.resize(_row_count * _column_count);
      throw;
    }

    _row_count += statements.size() / 2;
    _data_frame_end = _row_count;
  }

  save();

  // refresh_ui();
}

void DbSqlEditorHistory::update_timestamp(std::tm timestamp) {
  details_model()->datestamp(timestamp);
  write_only_details_model()->datestamp(timestamp);
}
//--------------------------------------------------------------------------------------------------
