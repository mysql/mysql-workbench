/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "sqlide_generics_private.h"

#include "recordset_be.h"
#include "recordset_data_storage.h"
#include "grt.h"
#include "cppdbc.h"
#include "grtui/binary_data_editor.h"

#include "mforms/menubar.h"
#include "mforms/toolbar.h"
#include "mforms/utilities.h"
#include "mforms/filechooser.h"

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/boost_smart_ptr_helpers.h"
#include "sqlite/command.hpp"
#include <fstream>
#include <sstream>
#include "grt/spatial_handler.h"

#include "recordset_text_storage.h"

DEFAULT_LOG_DOMAIN("Recordset")

using namespace bec;
using namespace base;

#define CATCH_AND_DISPATCH_EXCEPTION(rethrow, context)                                                          \
  catch (sql::SQLException & e) {                                                                               \
    rethrow ? throw                                                                                             \
            : task->send_msg(grt::ErrorMsg, strfmt("Error Code: %i\n%s", e.getErrorCode(), e.what()), context); \
  }                                                                                                             \
  catch (sqlite::database_exception & e) {                                                                      \
    rethrow ? throw : task->send_msg(grt::ErrorMsg, e.what(), context);                                         \
  }                                                                                                             \
  catch (std::exception & e) {                                                                                  \
    rethrow ? throw : task->send_msg(grt::ErrorMsg, e.what(), context);                                         \
  }

const std::string ERRMSG_PENDING_CHANGES = _("There are pending changes. Please commit or rollback first.");
std::string Recordset::_add_change_record_statement =
  "insert into `changes` (`record`, `action`, `column`) values (?, ?, ?)";

Recordset::ClientData::~ClientData() {
}

Recordset::Ref Recordset::create() {
  Ref instance(new Recordset());
  return instance;
}

Recordset::Ref Recordset::create(GrtThreadedTask::Ref parent_task) {
  Ref instance(new Recordset(parent_task));
  return instance;
}

static gint next_id = 0;

Recordset::Recordset()
  : VarGridModel(), _preserveRowFilters(false), _inserts_editor(false), task(GrtThreadedTask::create()) {
  _toolbar = NULL;
  _client_data = NULL;
  _context_menu = 0;
  _id = g_atomic_int_get(&next_id);
  g_atomic_int_inc(&next_id);

  task->desc("Recordset task");
  task->send_task_res_msg(false);
  apply_changes_cb = [this]() { apply_changes_(); };
  register_default_actions();
  reset();
}

Recordset::Recordset(GrtThreadedTask::Ref parent_task)
  : VarGridModel(), _inserts_editor(false), task(GrtThreadedTask::create(parent_task)) {
  _toolbar = NULL;
  _client_data = NULL;
  _context_menu = 0;
  _id = g_atomic_int_get(&next_id);
  g_atomic_int_inc(&next_id);

  task->send_task_res_msg(false);
  apply_changes_cb = [this]() { apply_changes_(); };
  register_default_actions();
  reset();
}

Recordset::~Recordset() {
  // recordset can't be freed before all calls planned from this class in main thread are finished
  bec::GRTManager::get()->get_dispatcher()->flush_pending_callbacks();
  delete _client_data;
  delete _context_menu;
}

bool Recordset::reset(Recordset_data_storage::Ptr data_storage_ptr, bool rethrow) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  VarGridModel::reset();

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();

  bool res = false;

  _aux_column_count = 0;
  _rowid_column = 0;
  _real_row_count = 0;
  _min_new_rowid = 0;
  _next_new_rowid = 0;
  _sort_columns.clear();
  _column_filter_expr_map.clear();
  _data_search_string.clear();

  RETAIN_WEAK_PTR(Recordset_data_storage, data_storage_ptr, data_storage)
  if (data_storage) {
    try {
      data_storage->do_unserialize(this, data_swap_db.get());
      rebuild_data_index(data_swap_db.get(), false, false);

      _column_count = _column_names.size();
      _aux_column_count = data_storage->aux_column_count();

      // add aux `id` column required by 2-level caching
      ++_aux_column_count;
      ++_column_count;
      _rowid_column = _column_count - 1;
      _column_names.push_back("id");
      _column_types.push_back(int());
      _real_column_types.push_back(int());
      _column_flags.push_back(0);

      {
        sqlite::query q(*data_swap_db, "select coalesce(max(id)+1, 0) from `data`");
        if (q.emit()) {
          std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(q.get_result());
          _min_new_rowid = rs->get_int(0);
        } else {
          _min_new_rowid = 0;
        }
        _next_new_rowid = _min_new_rowid;
      }

      recalc_row_count(data_swap_db.get());

      _readonly = data_storage->readonly();

      _readonly_reason = data_storage->readonly_reason();
      res = true;
    }
    CATCH_AND_DISPATCH_EXCEPTION(rethrow, "Reset recordset")
  }

  // Don't try to refresh the (non-existing) GUI when running unit tests.
  // The record set is freed before this callback is triggered.
  if (!grt::GRT::get()->testing()) {
    bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<void>(
      [this, data_swap_db]() {
        // We need to reapply filters once everything is loaded.
        if (_preserveRowFilters) {
          if (_toolbar != nullptr) {
            auto item = _toolbar->find_item("Search Field");
            if (item != nullptr) {
              _data_search_string = item->get_text();
              rebuild_data_index(data_swap_db.get(), true, false);
            }
          }
        } else {
          // Otherwise, we clear up toolbar.
          if (_toolbar != nullptr) {
            auto item = _toolbar->find_item("Search Field");
            if (item != nullptr)
              item->set_text("");
          }
        }
      }, false, false);
  }

  // Don't use refresh() to send update requests for the UI. It's regularly called from a background thread.
  // Instead the caller should schedule refresh calls (and coalesce them).

  return res;
}

void Recordset::reset() {
  reset(false);
}

bool Recordset::reset(bool rethrow) {
  return reset(_data_storage, rethrow);
}

bool Recordset::can_close() {
  return can_close(true);
}

bool Recordset::can_close(bool interactive) {
  bool res = !has_pending_changes();
  if (!res && interactive) {
    int r = mforms::Utilities::show_warning(
      _("Close Recordset"),
      strfmt(_("There are unsaved changes to the recordset data: %s. Do you want to apply them before closing?"),
             _caption.c_str()),
      _("Apply"), _("Cancel"), _("Don't Apply"));
    switch (r) {
      case mforms::ResultOk: // Apply
        apply_changes();
        res = !has_pending_changes();
        break;
      case mforms::ResultCancel:
        res = false;
        break;
      case mforms::ResultOther:
        res = true;
        break;
    }
  }
  return res;
}

bool Recordset::close() {
  RETVAL_IF_FAIL_TO_RETAIN_RAW_PTR(Recordset, this, false)
  on_close(weak_ptr_from(this));
  return true;
}

void Recordset::refresh() {
  if (has_pending_changes()) {
    task->send_msg(grt::ErrorMsg, ERRMSG_PENDING_CHANGES, _("Refresh Recordset"));
    return;
  }

  std::string data_search_string = _data_search_string;

  VarGridModel::refresh();
  reset();

  // reapply filter, if needed
  if (!data_search_string.empty())
    set_data_search_string(data_search_string);

  if (rows_changed)
    rows_changed();
}

void Recordset::rollback() {
  if (!reset(false))
    task->send_msg(grt::ErrorMsg, _("Rollback failed"), _("Rollback recordset changes"));
  else
    refresh_ui();
}

void Recordset::data_edited() {
  if (bec::GRTManager::get()->in_main_thread())
    data_edited_signal();
  else
    logError("data_edited called from thread\n");
}

RowId Recordset::real_row_count() const {
  return _real_row_count;
}

void Recordset::recalc_row_count(sqlite::connection *data_swap_db) {
  // row count (visible rows only, some can be filtered out by applied column filters)
  {
    sqlite::query q(*data_swap_db, "select count(*) from `data_index`");
    if (q.emit()) {
      std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(q.get_result());
      _row_count = rs->get_int(0);
    } else {
      _row_count = 0;
    }
  }

  // real row count (as if no column filters applied)
  {
    sqlite::query q(*data_swap_db, "select count(*) from `data`");
    if (q.emit()) {
      std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(q.get_result());
      _real_row_count = rs->get_int(0);
    } else {
      _real_row_count = 0;
    }
  }
}

Recordset::Cell Recordset::cell(RowId row, ColumnId column) {
  if (_row_count == row) {
    RowId rowid = _next_new_rowid++; // rowid of the new record
    {
      std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
      sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db.get());

      // insert new empty data record
      {
        std::list<sqlite::variant_t> bind_vars;
        bind_vars.push_back((int)rowid);
        emit_partition_commands(data_swap_db.get(), data_swap_db_partition_count(),
                                "insert into `data%s` (id) values (?)", bind_vars);
      }

      // insert new data index record
      {
        sqlite::command insert_data_index_record_statement(*data_swap_db, "insert into `data_index` (id) values (?)");
        insert_data_index_record_statement % (int)rowid;
        insert_data_index_record_statement.emit();
      }

      // log insert action
      {
        sqlite::command add_change_record_statement(*data_swap_db, _add_change_record_statement);
        add_change_record_statement % (int)rowid;
        add_change_record_statement % 1;
        static sqlite::null_type null_obj;
        add_change_record_statement % null_obj;
        add_change_record_statement.emit();
      }

      transaction_guarder.commit();
    }

    _data.resize(_data.size() + _column_count);
    ++_row_count;

    // init new row fields with null-values
    Cell new_cell = _data.begin() + (_data.size() - _column_count);
    for (ColumnId col = 0; _column_count > col; ++col, ++new_cell)
      *(new_cell) = sqlite::null_t();
    _data[_data.size() - _column_count + _rowid_column] = (int)rowid;

    if (rows_changed)
      rows_changed();
  }

  return VarGridModel::cell(row, column);
}

void Recordset::after_set_field(const NodeId &node, ColumnId column, const sqlite::variant_t &value) {
  VarGridModel::after_set_field(node, column, value);
  mark_dirty(node[0], column, value);
  data_edited();
  tree_changed();
}

void Recordset::mark_dirty(RowId row, ColumnId column, const sqlite::variant_t &new_value) {
  base::RecMutexLock data_mutex(_data_mutex);

  RowId rowid(row);
  NodeId node(row);
  if (get_field_(node, _rowid_column, (ssize_t &)rowid)) {
    std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
    sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db.get());

    // update record
    {
      size_t partition = data_swap_db_column_partition(column);
      std::string partition_suffix = data_swap_db_partition_suffix(partition);
      std::string sql =
        strfmt("update `data%s` set `_%u`=? where `id`=?", partition_suffix.c_str(), (unsigned int)column);
      sqlite::command update_data_record_statement(*data_swap_db, sql);
      sqlide::BindSqlCommandVar bind_sql_command_var(&update_data_record_statement);
      boost::apply_visitor(bind_sql_command_var, new_value);
      update_data_record_statement % (int)rowid;
      update_data_record_statement.emit();
    }

    // log update action
    {
      sqlite::command add_data_change_record_statement(*data_swap_db, _add_change_record_statement);
      add_data_change_record_statement % (int)rowid;
      add_data_change_record_statement % 0;
      add_data_change_record_statement % (int)column;
      add_data_change_record_statement.emit();
    }

    transaction_guarder.commit();
  }
}

std::string Recordset::caption() {
  return base::strfmt("%s%s", _caption.c_str(), has_pending_changes() ? "*" : "");
}

bool Recordset::delete_node(const bec::NodeId &node) {
  std::vector<bec::NodeId> nodes(1, node);
  return delete_nodes(nodes);
}

bool Recordset::delete_nodes(std::vector<bec::NodeId> &nodes) {
  {
    base::RecMutexLock data_mutex(_data_mutex);

    {
      std::sort(nodes.begin(), nodes.end());
      std::vector<bec::NodeId>::iterator i = std::unique(nodes.begin(), nodes.end());
      nodes.erase(i, nodes.end());
    }
    RowId processed_node_count = 0;

    for (auto &node : nodes) {
      RowId row = node[0] - processed_node_count;
      if (!node.is_valid() || (row >= _row_count))
        return false;
    }

    for (auto &node : nodes) {
      node[0] -= processed_node_count;
      RowId row = node[0];

      ssize_t rowid;
      if (get_field_(node, _rowid_column, rowid)) {
        std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
        sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db.get());

        // save copy of the record being deleted
        for (size_t partition = 0, partition_count = data_swap_db_partition_count(); partition < partition_count;
             ++partition) {
          std::string partition_suffix = data_swap_db_partition_suffix(partition);
          sqlite::command save_deleted_data_record_statement(
            *data_swap_db, strfmt("insert into `deleted_rows%s` select * from `data%s` where id=?",
                                  partition_suffix.c_str(), partition_suffix.c_str()));
          save_deleted_data_record_statement % (int)rowid;
          save_deleted_data_record_statement.emit();
        }

        // delete data record
        {
          std::list<sqlite::variant_t> bind_vars;
          bind_vars.push_back((int)rowid);
          emit_partition_commands(data_swap_db.get(), data_swap_db_partition_count(), "delete from `data%s` where id=?",
                                  bind_vars);
        }

        // delete data index record
        {
          sqlite::command delete_data_index_record_statement(*data_swap_db, "delete from `data_index` where id=?");
          delete_data_index_record_statement % (int)rowid;
          delete_data_index_record_statement.emit();
        }

        // log delete action
        {
          sqlite::command add_change_record_statement(*data_swap_db, _add_change_record_statement);
          add_change_record_statement % (int)rowid;
          add_change_record_statement % -1;
          static sqlite::null_type null_obj;
          add_change_record_statement % null_obj;
          add_change_record_statement.emit();
        }

        transaction_guarder.commit();

        --_row_count;
        --_data_frame_end;

        // delete record from cached data frame
        {
          Cell row_begin = _data.begin() + (row - _data_frame_begin) * _column_count;
          _data.erase(row_begin, row_begin + _column_count);
        }

        ++processed_node_count;
      }
    }

    nodes.clear();
  }

  if (rows_changed)
    rows_changed();

  data_edited();

  return true;
}

bool Recordset::has_pending_changes() {
  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  if (data_swap_db) {
    sqlite::query check_pending_changes_statement(*data_swap_db, "select exists(select 1 from `changes`)");
    std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(check_pending_changes_statement.emit_result());
    return (rs->get_int(0) == 1);
  } else {
    return false;
  }
}

void Recordset::pending_changes(int &upd_count, int &ins_count, int &del_count) const {
  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();

  std::string count_pending_changes_statement_sql =
    "select 1, (select count(*) from `data` where id>=?)\n"
    "union all\n"
    "select -1, (select count(*) from `deleted_rows` where id<?)\n"
    "union all\n"
    "select 0, (select count(1) from\n"
    "(select `record` from `changes` where `action`=0 and `record`<? group by `record`\n"
    "except\n"
    "select id from `deleted_rows`))";
  sqlite::query count_pending_changes_statement(*data_swap_db, count_pending_changes_statement_sql);
  count_pending_changes_statement % (int)_min_new_rowid;
  count_pending_changes_statement % (int)_min_new_rowid;
  count_pending_changes_statement % (int)_min_new_rowid;
  std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(count_pending_changes_statement.emit_result());
  do {
    switch (rs->get_int(0)) {
      case 0:
        upd_count = rs->get_int(1);
        break;
      case 1:
        ins_count = rs->get_int(1);
        break;
      case -1:
        del_count = rs->get_int(1);
        break;
    }
  } while (rs->next_row());
}

grt::StringRef Recordset::do_apply_changes(Ptr self_ptr, Recordset_data_storage::Ptr data_storage_ptr,
                                           bool skip_commit) {
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, self_ptr, self, grt::StringRef(""))
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset_data_storage, data_storage_ptr, data_storage, grt::StringRef(""))
  try {
    data_storage->apply_changes(self_ptr, skip_commit);
    if (skip_commit)
      task->send_msg(grt::InfoMsg, _("Apply complete"), _("Applied but did not commit recordset changes"));
    else
      task->send_msg(grt::InfoMsg, _("Apply complete"), _("Applied and commited recordset changes"));
    reset(data_storage_ptr, false);
  }
  CATCH_AND_DISPATCH_EXCEPTION(false, "Apply recordset changes")

  return grt::StringRef("");
}

/*
 * Actually applies recordset changes. Must run in the main thread for UI updates.
 */
void Recordset::apply_changes_(Recordset_data_storage::Ptr data_storage_ptr) {
  Recordset_data_storage::Ref storage = data_storage_ptr.lock();
  try {
    storage->apply_changes(weak_ptr_from(this), false);
    reset(data_storage_ptr, false);

    // This message is nowhere shown and only the unit tests take notice.
    task->send_msg(grt::InfoMsg, _("Apply complete"), _("Applied and commited recordset changes"));
    on_apply_changes_finished();
  }
  CATCH_AND_DISPATCH_EXCEPTION(false, "Apply recordset changes")
}

static int process_task_msg(int msgType, const std::string &message, const std::string &detail, int &error_count,
                            std::string &messages_out) {
  if (msgType == grt::ErrorMsg)
    error_count++;

  if (!message.empty()) {
    if (!messages_out.empty())
      messages_out.append("\n");
    messages_out.append(message);
  }
  return 0;
}

bool Recordset::apply_changes_and_gather_messages(std::string &messages) {
  int error_count = 0;
  GrtThreadedTask::Msg_cb cb(task->msg_cb());

  task->msg_cb(std::bind(process_task_msg, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                         std::ref(error_count), std::ref(messages)));
  apply_changes();
  task->msg_cb(cb);

  return error_count == 0;
}

void Recordset::rollback_and_gather_messages(std::string &messages) {
  int error_count = 0;
  GrtThreadedTask::Msg_cb cb(task->msg_cb());

  task->msg_cb(std::bind(process_task_msg, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                         std::ref(error_count), std::ref(messages)));
  rollback();
  task->msg_cb(cb);
}

int Recordset::on_apply_changes_finished() {
  task->finish_cb(GrtThreadedTask::Finish_cb());
  if (rows_changed)
    rows_changed();
  data_edited();
  return refresh_ui();
}

void Recordset::apply_changes_() {
  apply_changes_(_data_storage);
}

bool Recordset::limit_rows() {
  return (_data_storage ? _data_storage->limit_rows() : false);
}

void Recordset::limit_rows(bool value) {
  if (has_pending_changes()) {
    task->send_msg(grt::ErrorMsg, ERRMSG_PENDING_CHANGES, _("Limit Rows"));
    return;
  }

  if (_data_storage) {
    if (_data_storage->limit_rows() != value) {
      _data_storage->limit_rows(value);
      refresh();
    }
  }
}

void Recordset::toggle_limit_rows() {
  limit_rows(!limit_rows());
}

void Recordset::scroll_rows_frame_forward() {
  if (_data_storage) {
    _data_storage->scroll_rows_frame_forward();
    refresh();
  }
}

void Recordset::scroll_rows_frame_backward() {
  if (_data_storage && (_data_storage->limit_rows_offset() != 0)) {
    _data_storage->scroll_rows_frame_backward();
    refresh();
  }
}

int Recordset::limit_rows_count() {
  return (_data_storage ? _data_storage->limit_rows_count() : 0);
}

void Recordset::limit_rows_count(int value) {
  if (_data_storage)
    _data_storage->limit_rows_count(value);
}

bool Recordset::limit_rows_applicable() {
  if (_data_storage && !_data_storage->limit_rows_applicable())
    return false;

  bool limit_rows_ = limit_rows();
  size_t limit_rows_count_ = limit_rows_count();
  size_t row_count_ = real_row_count();
  return (limit_rows_ && (row_count_ == limit_rows_count_)) || (!limit_rows_ && (row_count_ > limit_rows_count_)) ||
         (0 < _data_storage->limit_rows_offset());
}

Recordset_data_storage::Ref Recordset::data_storage_for_export(const std::string &format) {
  _data_storage_for_export.reset();

  {
    std::vector<Recordset_storage_info> storage_types(Recordset_text_storage::storage_types());
    for (std::vector<Recordset_storage_info>::const_iterator i = storage_types.begin(); i != storage_types.end(); ++i) {
      if (i->name == format) {
        Recordset_text_storage::Ref ds(Recordset_text_storage::create());
        ds->data_format(format);
        _data_storage_for_export = ds;
        break;
      }
    }
  }

  if (_data_storage_for_export)
    return _data_storage_for_export;
  throw std::runtime_error(strfmt("Data storage format is not supported: %s", format.c_str()));
}

std::vector<Recordset_storage_info> Recordset::data_storages_for_export() {
  std::vector<Recordset_storage_info> storage_types;

  storage_types = Recordset_text_storage::storage_types();

  return storage_types;
}

void Recordset::sort_by(ColumnId column, int direction, bool retaining) {
  if (_column_count == 0)
    return;

  if (!retaining) {
    _sort_columns.clear();
    if (!(direction)) {
      std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
      rebuild_data_index(data_swap_db.get(), true, true);

      refresh_ui(); // refresh the sort indicators in column headers
      return;
    }
  }

  bool sort_column_exists = false;
  bool is_resort_needed = true;
  for (SortColumns::iterator sort_column = _sort_columns.begin(), end = _sort_columns.end(); sort_column != end;
       ++sort_column) {
    if (sort_column->first == column) {
      if ((direction)) {
        sort_column->second = direction;
        sort_column_exists = true;
      } else {
        if (_sort_columns.rbegin()->first == column)
          is_resort_needed = false;
        _sort_columns.erase(sort_column);
      }
      break;
    }
  }
  if (!sort_column_exists && (direction))
    _sort_columns.push_back(std::make_pair(column, direction));

  if (!is_resort_needed || _sort_columns.empty())
    return;

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  rebuild_data_index(data_swap_db.get(), true, true);
}

std::string Recordset::get_column_filter_expr(ColumnId column) const {
  Column_filter_expr_map::const_iterator i = _column_filter_expr_map.find(column);
  if (i != _column_filter_expr_map.end())
    return i->second;
  return "";
}

bool Recordset::has_column_filters() const {
  return !_column_filter_expr_map.empty();
}

bool Recordset::has_column_filter(ColumnId column) const {
  Column_filter_expr_map::const_iterator i = _column_filter_expr_map.find(column);
  return (i != _column_filter_expr_map.end());
}

void Recordset::reset_column_filters() {
  _column_filter_expr_map.clear();

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  rebuild_data_index(data_swap_db.get(), true, true);
}

void Recordset::reset_column_filter(ColumnId column) {
  Column_filter_expr_map::iterator i = _column_filter_expr_map.find(column);
  if (i == _column_filter_expr_map.end())
    return;
  _column_filter_expr_map.erase(i);

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  rebuild_data_index(data_swap_db.get(), true, true);
}

void Recordset::set_column_filter(ColumnId column, const std::string &filter_expr) {
  if (column >= get_column_count())
    return;
  Column_filter_expr_map::const_iterator i = _column_filter_expr_map.find(column);
  if ((i != _column_filter_expr_map.end()) && (i->second == filter_expr))
    return;
  _column_filter_expr_map[column] = filter_expr;

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  rebuild_data_index(data_swap_db.get(), true, true);
}

size_t Recordset::column_filter_icon_id() const {
  IconManager *icon_man = IconManager::get_instance();
  return icon_man->get_icon_id("tiny_search.png");
}

const std::string &Recordset::data_search_string() const {
  return _data_search_string;
}

void Recordset::set_data_search_string(const std::string &value) {
  if (value == _data_search_string)
    return;
  _data_search_string = value;

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  rebuild_data_index(data_swap_db.get(), true, true);
}

void Recordset::reset_data_search_string() {
  if (_data_search_string.empty())
    return;
  _data_search_string.clear();

  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
  rebuild_data_index(data_swap_db.get(), true, true);
}

void Recordset::rebuild_data_index(sqlite::connection *data_swap_db, bool do_cache_data_frame, bool do_refresh_ui) {
  {
    base::RecMutexLock data_mutex(_data_mutex);

    std::string where_clause;
    {
      sqlide::QuoteVar qv;
      {
        qv.escape_string = std::bind(sqlide::QuoteVar::escape_ansi_sql_string, std::placeholders::_1);
        qv.store_unknown_as_string = true;
        qv.allow_func_escaping = false;
      }
      sqlite::variant_t var_string_type = std::string();
      sqlite::variant_t var_string;
      std::string sql_string;

      // column filters subclause
      std::string where_subclause1;
      {
        for (auto &column_filter_expr : _column_filter_expr_map) {
          var_string = column_filter_expr.second;
          sql_string = boost::apply_visitor(qv, var_string_type, var_string);
          where_subclause1 += strfmt("_%u like %s and ", (unsigned int)column_filter_expr.first, sql_string.c_str());
        }
        if (!where_subclause1.empty()) {
          where_subclause1.resize(where_subclause1.size() - std::string(" and ").size());
          where_subclause1.insert(0, "(");
          where_subclause1.append(")");
        }
      }

      // data search subclause
      std::string where_subclause2;
      if (!_data_search_string.empty()) {
        var_string = "%" + _data_search_string + "%";
        sql_string = boost::apply_visitor(qv, var_string_type, var_string);
        for (ColumnId column = 0, column_count = get_column_count(); column < column_count; ++column) {
          where_subclause2 += strfmt("_%u like %s or ", (unsigned int)column, sql_string.c_str());
        }
        if (!where_subclause2.empty()) {
          where_subclause2.resize(where_subclause2.size() - std::string(" or ").size());
          where_subclause2.insert(0, "(");
          where_subclause2.append(")");
        }
      }

      if (!where_subclause1.empty() || !where_subclause2.empty()) {
        std::string subclauses_mediator = (!where_subclause1.empty() && !where_subclause2.empty()) ? " and " : "";
        where_clause =
          strfmt("where %s%s%s", where_subclause1.c_str(), subclauses_mediator.c_str(), where_subclause2.c_str());
      }
    }

    std::string orderby_clause;
    {
      for (auto &sort_column : _sort_columns) {
        std::string column_expr;
        switch (get_real_column_type(sort_column.first)) {
          case NumericType:
          case FloatType:
          case DatetimeType:
            column_expr = strfmt("cast(_%u as numeric)", (unsigned int)sort_column.first);
            break;
          case StringType:
            column_expr = strfmt("_%u COLLATE NOCASE", (unsigned int)sort_column.first);
            break;

          default:
            column_expr = strfmt("_%u", (unsigned int)sort_column.first);
            break;
        }
        const char *dir;
        switch (sort_column.second) {
          case 1:
            dir = "ASC";
            break;
          case -1:
            dir = "DESC";
            break;
          default:
            dir = "";
            break;
        }
        orderby_clause += strfmt("%s %s, ", column_expr.c_str(), dir);
      }
      if (!orderby_clause.empty()) {
        orderby_clause.resize(orderby_clause.size() - std::string(", ").size());
        orderby_clause.insert(0, "order by ");
      }
    }

    std::string tables_join = "`data`";
    {
      for (size_t partition = 1, partition_count = data_swap_db_partition_count(); partition < partition_count;
           ++partition) {
        std::string partition_suffix = data_swap_db_partition_suffix(partition);
        tables_join +=
          strfmt(" inner join `data%s` on (`data`.id=`data%s`.id)", partition_suffix.c_str(), partition_suffix.c_str());
      }
    }

    {
      sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db);

      std::string temp_table_name = "`data_index_" + grt::get_guid() + "`";

      sqlite::execute(*data_swap_db, strfmt("create table if not exists %s (`id` integer)", temp_table_name.c_str()),
                      true);
      sqlite::execute(*data_swap_db, strfmt("insert into %s select `data`.`id` from %s %s %s", temp_table_name.c_str(),
                                            tables_join.c_str(), where_clause.c_str(), orderby_clause.c_str()),
                      true);
      sqlite::execute(*data_swap_db, "drop table if exists `data_index`", true);
      sqlite::execute(*data_swap_db, strfmt("alter table %s rename to `data_index`", temp_table_name.c_str()), true);

      transaction_guarder.commit();
    }

    recalc_row_count(data_swap_db);

    if (do_cache_data_frame && _column_count > 0)
      cache_data_frame(0, true);
  }

  if (do_refresh_ui)
    refresh_ui();
}

void Recordset::paste_rows_from_clipboard(ssize_t dest_row) {
  std::string text = mforms::Utilities::get_clipboard_text();
  std::vector<std::string> rows;

  if (text.find("\r\n") != std::string::npos)
    rows = base::split(text, "\r\n");
  else
    rows = base::split(text, "\n");

  if (rows.empty())
    return;

  if (rows.back().empty())
    rows.pop_back();

  if (dest_row < 0 || dest_row == (ssize_t)count() - 1)
    dest_row = count() - 1;
  else {
    if (rows.size() > 1) {
      if (mforms::Utilities::show_message_and_remember(
            "Paste Rows", "Cannot paste more than one row into an existing row, would you like to append them?",
            "Append", "Cancel", "", "Recordset.appendMultipleRowsOnPaste", "") != mforms::ResultOk)
        return;
      dest_row = count() - 1;
    }
  }

  int separator = ',';
  if (text.find('\t') != std::string::npos)
    separator = '\t';

  for (std::vector<std::string>::const_iterator row = rows.begin(); row != rows.end(); ++row) {
    if (!row->empty()) {
      std::vector<std::string> parts = base::split_token_list(*row, separator);

      if (parts.size() != get_column_count()) {
        mforms::Utilities::show_error(
          "Cannot Paste Row",
          strfmt("Number of fields in pasted data doesn't match the columns in the table (%zi vs %zi).\n"
                 "Data must be in the same format used by the Copy Row Content command.",
                 parts.size(), get_column_count()),
          "OK");

        if (rows_changed && row != rows.begin())
          rows_changed();

        return;
      }
      int i = 0;
      for (std::vector<std::string>::const_iterator p = parts.begin(); p != parts.end(); ++p, ++i) {
        std::string token = base::trim(*p);
        if (token == "NULL")
          set_field_null(dest_row, i);
        else {
          if (!token.empty() && token[0] == '\'' && token[token.size() - 1] == '\'')
            token = token.substr(1, token.size() - 2);
          set_field(dest_row, i, base::unescape_sql_string(token, '\''));
        }
      }
      dest_row++;
    }
  }

  if (rows_changed)
    rows_changed();
}

void Recordset::showPointInBrowser(const bec::NodeId &node, ColumnId column) {
  base::RecMutexLock data_mutex(_data_mutex);
  if (sqlide::is_var_blob(_real_column_types[column])) {
    std::string geometry;
    if (get_raw_field(node, column, geometry) && !geometry.empty()) {
      spatial::Importer importer;
      if (!importer.import_from_mysql(geometry)) {
        if (importer.getType() == spatial::ShapePoint) {
          std::deque<spatial::ShapeContainer> tmpShapes;
          importer.get_points(tmpShapes);
          if (tmpShapes.size() == 1 && tmpShapes[0].points.size() == 1) {
            std::string url = bec::GRTManager::get()->get_app_option_string("SqlEditor:geographicLocationURL");
            if (url.empty()) {
              logError("Got empty url when trying to access geographicLocationURL\n");
              mforms::Utilities::show_error(
                  "Invalid Browser Location",
                  "Point URL option have to be specified in the preferences to use this functionality.", "OK");
              return;
            }

            url = base::replaceString(url, "%LAT%", base::to_string(tmpShapes[0].points[0].y));
            url = base::replaceString(url, "%LON%", base::to_string(tmpShapes[0].points[0].x));
            logDebug3("Opening url: %s\n", url.c_str());
            mforms::Utilities::open_url(url);

          } else {
            logDebug3("Invalid column specified to showPointInBrowser.\n");
            mforms::Utilities::show_error("Invalid Column",
                                          "A geometry type column is required to use this functionality.", "OK");
          }
        } else {
          logError("Invalid column specified to showPointInBrowser, expected POINT got %s.\n",
                   importer.getName().c_str());
          mforms::Utilities::show_error("Invalid Column", "This functionality works only with Points", "OK");
        }
      } else {
        logError("Unable to load geometry data\n");
        mforms::Utilities::show_error("Invalid Column", "Unable to load geometry data", "OK");
      }
    }
  } else {
    logDebug3("Invalid column specified to show point in browser\n");
    mforms::Utilities::show_error("Invalid Column", "A geometry type column is required to use this functionality.",
                                  "OK");
  }
}

mforms::ContextMenu *Recordset::get_context_menu() {
  if (!_context_menu)
    _context_menu = new mforms::ContextMenu();
  return _context_menu;
}

void Recordset::update_selection_for_menu(const std::vector<int> &rows, int clicked_column) {
  // TODO: lift the restriction to a single column.
  //       We need to support multiple cells (in multiple columns) on all platforms.
  _selected_rows = rows;
  _selected_column = clicked_column;

  if (_context_menu) {
    _context_menu->remove_all();

    bool ro = is_readonly();

    mforms::MenuItem *item = mforms::manage(new mforms::MenuItem(ro ? "Open Value in Viewer" : "Open Value in Editor"));
    item->set_name("Edit Cell"); // action in higher level
    item->setInternalName("edit_cell");

    item->set_enabled((rows.size() == 1) && (clicked_column >= 0));
    if (item->get_enabled()) {
      switch (get_real_column_type(clicked_column)) {
        case StringType:
        case BlobType:
          break;
        default:
          item->set_enabled(false);
          break;
      }
    }
    _context_menu->add_item(item);

    if (clicked_column >= 0 && isGeometry(clicked_column)) {
      item = _context_menu->add_item_with_title("Show point in browser",
                                                std::bind(&Recordset::activate_menu_item, this, "show_in_browser", rows, clicked_column),
                                                "Show point in browser", "show_in_browser");
    }

    _context_menu->add_separator();

    item = _context_menu->add_item_with_title(
      "Set Field to NULL", std::bind(&Recordset::activate_menu_item, this, "set_to_null", rows, clicked_column),
      "Set Field to NULL", "set_to_null");

// On Windows we can select individual cells, so it is perfectly ok to allow acting on multiple
// cells. The other platforms only select entire rows in the multi-selection case.
// So we to have disallow acting on them to avoid changing unrelated entries.
#ifdef _MSC_VER
    item->set_enabled(clicked_column >= 0 && !ro);
#else
    item->set_enabled(clicked_column >= 0 && rows.size() == 1 && !ro);
#endif

    item = _context_menu->add_item_with_title(
      "Mark Field Value as a Function/Literal",
      std::bind(&Recordset::activate_menu_item, this, "set_to_function", rows, clicked_column), "Mark Fiels as Function or Literal", "set_to_function");
#ifdef _MSC_VER
    item->set_enabled(clicked_column >= 0 && !ro);
#else
    item->set_enabled(clicked_column >= 0 && rows.size() == 1 && !ro);
#endif

    item = _context_menu->add_item_with_title(
      "Delete Row(s)", std::bind(&Recordset::activate_menu_item, this, "delete_row", rows, clicked_column),
      "Delete Rows", "delete_row");
    item->set_enabled(rows.size() > 0 && !ro);

    _context_menu->add_separator();

    item = _context_menu->add_item_with_title(
      "Load Value From File...",
      std::bind(&Recordset::activate_menu_item, this, "load_from_file", rows, clicked_column), "Load Value From File", "load_from_file");
    item->set_enabled(clicked_column >= 0 && rows.size() == 1 && !ro);

    item = _context_menu->add_item_with_title(
      "Save Value To File...", std::bind(&Recordset::activate_menu_item, this, "save_to_file", rows, clicked_column),
      "Save Value To File", "save_to_file");
    item->set_enabled(clicked_column >= 0 && rows.size() == 1 && !ro);

    _context_menu->add_separator();

    item = _context_menu->add_item_with_title(
      "Copy Row", std::bind(&Recordset::activate_menu_item, this, "copy_row", rows, clicked_column), "Copy Row", "copy_row");
    item->set_enabled(rows.size() > 0);
    item = _context_menu->add_item_with_title(
      "Copy Row (with names)",
      std::bind(&Recordset::activate_menu_item, this, "copy_row_with_names", rows, clicked_column),
      "Copy Row With Names", "copy_row_with_names");

    item = _context_menu->add_item_with_title(
      "Copy Row (unquoted)", std::bind(&Recordset::activate_menu_item, this, "copy_row_unquoted", rows, clicked_column),
      "Copy Row Unquoted", "copy_row_unquoted");
    item->set_enabled(rows.size() > 0);
    item = _context_menu->add_item_with_title(
      "Copy Row (with names, unquoted)",
      std::bind(&Recordset::activate_menu_item, this, "copy_row_unquoted_with_names", rows, clicked_column),
      "Copy Row With Names and Unquoted)", "copy_row_unquoted_with_names");

    item = _context_menu->add_item_with_title(
      "Copy Row (with names, tab separated)",
      std::bind(&Recordset::activate_menu_item, this, "copy_row_with_names_tabsep", rows, clicked_column),
      "Copy Row with Names Tab Separated", "copy_row_with_names_tabsep");
    item->set_enabled(rows.size() > 0);
    
    item = _context_menu->add_item_with_title(
      "Copy Row (tab separated)",
      std::bind(&Recordset::activate_menu_item, this, "copy_row_tabsep", rows, clicked_column),
      "Copy Row Tab Separated", "copy_row_tabsep");
    item->set_enabled(rows.size() > 0);

    item = _context_menu->add_item_with_title(
      "Copy Field", std::bind(&Recordset::activate_menu_item, this, "copy_field", rows, clicked_column), "Copy Field", "copy_field");
    item->set_enabled(clicked_column >= 0 && rows.size() == 1);

    item = _context_menu->add_item_with_title(
      "Copy Field (unquoted)",
      std::bind(&Recordset::activate_menu_item, this, "copy_field_unquoted", rows, clicked_column),
      "Copy Field (unquoted)", "copy_field_unquoted");
    item->set_enabled(clicked_column >= 0 && rows.size() == 1);

    item = _context_menu->add_item_with_title(
      "Paste Row", std::bind(&Recordset::activate_menu_item, this, "paste_row", rows, clicked_column), "Paste Row", "paste_row");
    item->set_enabled(rows.size() <= 1 && !mforms::Utilities::get_clipboard_text().empty() && !ro);

    if (update_selection_for_menu_extra)
      update_selection_for_menu_extra(_context_menu, rows, clicked_column);
  }
}

void Recordset::activate_menu_item(const std::string &action, const std::vector<int> &rows, int clicked_column) {
  bool need_ui_refresh = false;

  // TODO: the tests here for rows count and clicked_column are all unnecessary. This has already be done.
  if (action == "edit_cell") {
    if (rows.size() == 1 && clicked_column >= 0) {
      open_field_data_editor(rows[0], clicked_column, "");
    }
  } else if (action == "set_to_null") {
    for (size_t i = 0; i < rows.size(); ++i) {
      bec::NodeId node;
      node.append(rows[i]);
      set_field_null(node, clicked_column);
    }
  } else if (action == "set_to_function") {
    for (size_t i = 0; i < rows.size(); ++i) {
      bec::NodeId node;
      Cell cell;

      node.append(rows[i]);
      std::string function;
      if (!get_cell(cell, node, clicked_column, false))
        function = "";
      else
        function = boost::apply_visitor(_var_to_str, *cell);
      if (!g_str_has_prefix(function.c_str(), "\\func"))
        set_field(node, clicked_column, std::string("\\func ") + function);
    }
  } else if (action == "delete_row") {
    if (rows.size() > 0) {
      std::vector<int> sorted_rows(rows);
      std::sort(sorted_rows.begin(), sorted_rows.end());
      std::vector<bec::NodeId> nodes;
      for (std::vector<int>::reverse_iterator iter = sorted_rows.rbegin(); iter != sorted_rows.rend(); ++iter)
        nodes.push_back(bec::NodeId(*iter));
      delete_nodes(nodes);
      need_ui_refresh = true;
    }
  } else if (action == "save_to_file") {
    if (rows.size() == 1 && clicked_column >= 0) {
      bec::NodeId node;
      node.append(rows[0]);
      save_to_file(node, clicked_column);
    }
  } else if (action == "load_from_file") {
    if (rows.size() == 1 && clicked_column >= 0) {
      bec::NodeId node;
      node.append(rows[0]);
      load_from_file(node, clicked_column);
    }
  } else if (action == "copy_row") {
    if (rows.size() > 0) {
      copy_rows_to_clipboard(rows, ", ");
    }
  } else if (action == "copy_row_with_names") {
    copy_rows_to_clipboard(rows, ", ", true, true);
  } else if (action == "copy_row_unquoted") {
    if (rows.size() > 0) {
      copy_rows_to_clipboard(rows, ", ", false);
    }
  } else if (action == "copy_row_unquoted_with_names") {
    copy_rows_to_clipboard(rows, ", ", false, true);
  }  else if (action == "copy_row_with_names_tabsep") {
    copy_rows_to_clipboard(rows, "\t", false, true);
  } else if (action == "copy_row_tabsep") {
    if (rows.size() > 0) {
      copy_rows_to_clipboard(rows, "\t", false);
    }
  } else if (action == "copy_field") {
    if (rows.size() == 1 && clicked_column >= 0) {
      copy_field_to_clipboard(rows[0], clicked_column);
    }
  } else if (action == "copy_field_unquoted") {
    if (rows.size() == 1 && clicked_column >= 0) {
      copy_field_to_clipboard(rows[0], clicked_column, false);
    }
  } else if (action == "paste_row") {
    paste_rows_from_clipboard(rows.empty() ? -1 : rows[0]);
    need_ui_refresh = true;
  }
  else if (action == "show_in_browser")
  {
    if (rows.size() == 1 && clicked_column >= 0)
    {
      bec::NodeId node;
      node.append(rows[0]);
      showPointInBrowser(node, clicked_column);
    }
  }

  if (need_ui_refresh)
    refresh_ui();
}

void Recordset::copy_rows_to_clipboard(const std::vector<int> &indeces, std::string sep, bool quoted,
                                       bool with_header) {
  ColumnId editable_col_count = get_column_count();
  if (!editable_col_count)
    return;

  sqlide::QuoteVar qv;
  {
    qv.escape_string = std::bind(base::escape_sql_string, std::placeholders::_1, false);
    qv.store_unknown_as_string = true;
    qv.allow_func_escaping = true;
  }

  Cell cell;
  std::string text;

  if (with_header) {
    text = "# ";
    for (ColumnId col = 0; editable_col_count > col; ++col) {
      if (col > 0)
        text.append(sep);
      text.append(get_column_caption(col));
    }
    text.append("\n");
  }

  for (auto row : indeces) {
    std::string line;
    for (ColumnId col = 0; editable_col_count > col; ++col) {
      bec::NodeId node(row);
      if (!get_cell(cell, node, col, false))
        continue;
      if (col > 0)
        line += sep;
      if (quoted)
        line += boost::apply_visitor(qv, _column_types[col], *cell);
      else
        line += boost::apply_visitor(_var_to_str, *cell);
    }
    if (!line.empty())
      text += line + "\n";
  }
  mforms::Utilities::set_clipboard_text(text);
}

void Recordset::copy_field_to_clipboard(int row, ColumnId column, bool quoted) {
  sqlide::QuoteVar qv;
  {
    qv.escape_string = std::bind(sqlide::QuoteVar::escape_ansi_sql_string, std::placeholders::_1);
    qv.store_unknown_as_string = true;
    qv.allow_func_escaping = true;
  }
  std::string text;
  bec::NodeId node(row);
  Cell cell;
  if (get_cell(cell, node, column, false)) {
    if (quoted)
      text = boost::apply_visitor(qv, _column_types[column], *cell);
    else
      text = boost::apply_visitor(_var_to_str, *cell);
  }
  mforms::Utilities::set_clipboard_text(text);
}

std::string Recordset::status_text() {
  std::string limit_text;

  if (limit_rows_applicable() && limit_rows())
    limit_text = ", row LIMIT active";
  else
    limit_text = "";

  std::string skipped_row_count_text;
  if (_data_storage && _data_storage->limit_rows()) {
    int limit_rows_offset = _data_storage->limit_rows_offset();
    if (limit_rows_offset > 0)
      skipped_row_count_text = strfmt(" after %i skipped", limit_rows_offset);
  }

  std::stringstream out;
  out << "Fetched " << real_row_count() << " records" << skipped_row_count_text << limit_text;
  std::string status_text = out.str();
  {
    int upd_count = 0, ins_count = 0, del_count = 0;
    pending_changes(upd_count, ins_count, del_count);
    if (upd_count > 0)
      status_text += strfmt(", updated %i", upd_count);
    if (ins_count > 0)
      status_text += strfmt(", inserted %i", ins_count);
    if (del_count > 0)
      status_text += strfmt(", deleted %i", del_count);
  }
  status_text.append(".");
  if (!status_text_trailer.empty())
    status_text.append(" ").append(status_text_trailer);

  return status_text;
}

static mforms::ToolBarItem *add_toolbar_action_item(mforms::ToolBar *toolbar, bec::IconManager *im,
                                                    const std::string &accessibilityName, const std::string &item_icon,
                                                    const std::string &item_name, const std::string &item_tooltip) {
  mforms::ToolBarItem *item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name(accessibilityName);
  item->setInternalName(item_name);
  item->set_icon(im->get_icon_path(item_icon));
  item->set_tooltip(item_tooltip);
  toolbar->add_item(item);
  return item;
}

static mforms::ToolBarItem *add_toolbar_action_item(mforms::ToolBar *toolbar, bec::IconManager *im,
                                                    const std::string &accessibilityName, const std::string &item_name,
                                                    const std::string &item_tooltip) {
  return add_toolbar_action_item(toolbar, im, accessibilityName, item_name + ".png", item_name, item_tooltip);
}

static void add_toolbar_label_item(mforms::ToolBar *toolbar, const std::string &label, const std::string &name) {
  mforms::ToolBarItem *item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
  
  item->set_text(label);
  item->set_name(name);
  toolbar->add_item(item);
}

void Recordset::search_activated(mforms::ToolBarItem *item) {
  std::string text;
  if ((text = item->get_text()).empty())
    reset_data_search_string();
  else
    set_data_search_string(text);
}

void Recordset::rebuild_toolbar() {
  if (_toolbar) {
    _toolbar->remove_all();
    // hack so that this label only appears for resultset grids and not inserts editor
    if (!_inserts_editor) {
      mforms::ToolBarItem *item = mforms::manage(new mforms::ToolBarItem(mforms::TitleItem));
      ;
      item->set_text("Result Grid");
      _toolbar->add_item(item);
      _toolbar->add_separator_item();
    }

    mforms::ToolBarItem *item;
    bec::IconManager *im = bec::IconManager::get_instance();

    item =
      add_toolbar_action_item(_toolbar, im, "Reset Record Sort", "record_sort_reset.png", "record_sort_reset", "Resets all sorted columns");

    if (!_data_storage || _data_storage->reloadable()) {
      item = add_toolbar_action_item(_toolbar, im, "Refresh", "record_refresh.png", "record_refresh",
                                     "Refresh data re-executing the original query");
      item->signal_activated()->connect(std::bind(&Recordset::refresh, this));
    }

    add_toolbar_label_item(_toolbar, "Filter Rows:", "Filter Rows");

    item = mforms::manage(new mforms::ToolBarItem(mforms::SearchFieldItem));
    item->set_name("Search Field");
    item->signal_activated()->connect(std::bind(&Recordset::search_activated, this, std::placeholders::_1));
    _toolbar->add_item(item);

    if (!is_readonly() || _inserts_editor) {
      _toolbar->add_separator_item();
      add_toolbar_label_item(_toolbar, "Edit:", "Edit");
      add_toolbar_action_item(_toolbar, im, "Edit Row", "record_edit", "Edit current row");    // connect in frontend
      add_toolbar_action_item(_toolbar, im, "Insert Row", "record_add", "Insert new row");       // connect in frontend
      add_toolbar_action_item(_toolbar, im, "Delete Selected", "record_del", "Delete selected rows"); // connect in frontend
    }
    _toolbar->add_separator_item();
    if (!is_readonly() || _inserts_editor)
      add_toolbar_label_item(_toolbar, "Export/Import:", "Export or Import");
    else
      add_toolbar_label_item(_toolbar, "Export:", "Export or Import");
    add_toolbar_action_item(_toolbar, im, "Export Record", "record_export",
                            "Export recordset to an external file");
    if (!is_readonly() || _inserts_editor)
      add_toolbar_action_item(_toolbar, im, "Record Import", "record_import",
                              "Import records from an external file");

#ifndef __APPLE__
    _toolbar->add_separator_item();
    add_toolbar_label_item(_toolbar, "Wrap Cell Content:", "Wrap Cell Content");
    add_toolbar_action_item(_toolbar, im, "Wrap Vertical", "record_wrap_vertical",
                            "Toggle wrapping of cell contents"); // connect in frontend
#endif

    if (limit_rows_applicable()) {
      _toolbar->add_separator_item();
      add_toolbar_label_item(_toolbar, "Fetch rows:", "Fetch Rows");
      item = add_toolbar_action_item(_toolbar, im, "Fetch Previous", "record_fetch_prev.png",
                                     "Fetch previous frame of records from the data source");
      item->signal_activated()->connect(std::bind(&Recordset::scroll_rows_frame_backward, this));
      item = add_toolbar_action_item(_toolbar, im, "Fetch Next", "record_fetch_next.png", "scroll_rows_frame_forward",
                                     "Fetch next frame of records from the data source");
      item->signal_activated()->connect(std::bind(&Recordset::scroll_rows_frame_forward, this));
    }

    if (_inserts_editor /* && !is_readonly()*/) {
      _toolbar->add_separator_item();
      add_toolbar_label_item(_toolbar, "Apply changes:", "Apply Changes");
      item = add_toolbar_action_item(_toolbar, im, "Save Record", "record_save", "Apply changes to data");
      item->signal_activated()->connect(std::bind(&Recordset::apply_changes, this));
      item = add_toolbar_action_item(_toolbar, im, "Discard Record", "record_discard", "Discard changes to data");
      item->signal_activated()->connect(std::bind(&Recordset::rollback, this));
    }
  }
}

mforms::ToolBar *Recordset::get_toolbar() {
  if (!_toolbar) {
    _toolbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
    rebuild_toolbar();
  }

  return _toolbar;
}

void Recordset::apply_changes() {
  if (flush_ui_changes_cb)
    flush_ui_changes_cb();

  apply_changes_cb();

  // If the SQL IDE redirects apply_changes_cb() we won't get a call to the task finish callback.
  // This causes some other notifications not to be called (especially changed rows).
  // Currently this callback is always redirected, so we can assume rows_changed() is not called multiple times.
  if (rows_changed)
    rows_changed();
}

ActionList &Recordset::action_list() {
  return _action_list;
}

void Recordset::register_default_actions() {
  _action_list.register_action("record_sort_reset", std::bind(&Recordset::sort_by, this, 0, 0, false));

  _action_list.register_action("scroll_rows_frame_forward", std::bind(&Recordset::scroll_rows_frame_forward, this));

  _action_list.register_action("scroll_rows_frame_backward", std::bind(&Recordset::scroll_rows_frame_backward, this));

  _action_list.register_action("record_fetch_all", std::bind(&Recordset::toggle_limit_rows, this));

  _action_list.register_action("record_refresh", std::bind(&Recordset::refresh, this));
}

class DataEditorSelector : public boost::static_visitor<BinaryDataEditor *> {
public:
  DataEditorSelector(bool read_only) : _read_only(read_only) {
  }
  DataEditorSelector(bool read_only, const std::string &encoding, const std::string &type)
    : _encoding(encoding), _type(type), _read_only(read_only) {
  }
  const std::string &encoding() const {
    return _encoding;
  }
  void encoding(const std::string &value) {
    _encoding = value;
  }

private:
  std::string _encoding;
  std::string _type;
  bool _read_only;

public:
  result_type operator()(const sqlite::null_t &v) {
    return new BinaryDataEditor(NULL, 0, _encoding, _type, _read_only);
  }
  result_type operator()(const std::string &v) {
    return new BinaryDataEditor(v.c_str(), v.length(), _encoding, _type, _read_only);
  }
  result_type operator()(const sqlite::blob_ref_t &v) {
    return new BinaryDataEditor(((!v || v->empty()) ? NULL : (const char *)&(*v)[0]), v->size(), _encoding, _type,
                                _read_only);
  }
  template <typename V>
  result_type operator()(const V &v) {
    return NULL;
  }
};

class DataEditorSelector2 : public boost::static_visitor<BinaryDataEditor *> {
public:
  DataEditorSelector2(bool read_only, const std::string &type) : _type(type), _read_only(read_only) {
  }

private:
  std::string _type;
  bool _read_only;

public:
  template <typename V>
  result_type operator()(const std::string &t, const V &v) {
    return DataEditorSelector(_read_only, "UTF-8", _type)(v);
  }
  template <typename V>
  result_type operator()(const sqlite::blob_ref_t &t, const V &v) {
    return DataEditorSelector(_read_only, "LATIN1", _type)(v);
  }
  template <typename T, typename V>
  result_type operator()(const T &r, const V &v) {
    // return NULL;
    // For unknown types treat them for now as string values. Since we have a binary editor pane there that should work
    // all the time well enough.
    return DataEditorSelector(_read_only, "UTF-8", _type)(v);
  }
};

void Recordset::open_field_data_editor(RowId row, ColumnId column, const std::string &logical_type) {
  base::RecMutexLock data_mutex(_data_mutex);

  try {
    sqlite::variant_t blob_value;
    sqlite::variant_t *value;

    if (sqlide::is_var_blob(_real_column_types[column])) {
      if (!_data_storage)
        return;
      RowId rowid;
      NodeId node(row);
      if (!get_field_(node, _rowid_column, (ssize_t &)rowid))
        return;
      std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
      _data_storage->fetch_blob_value(this, data_swap_db.get(), rowid, column, blob_value);
      value = &blob_value;
    } else {
      Cell cell;
      bec::NodeId node(row);
      if (!get_cell(cell, node, column, false))
        return;
      value = &(*cell);
    }

    DataEditorSelector2 data_editor_selector2(is_readonly(), logical_type);
    BinaryDataEditor *data_editor = boost::apply_visitor(data_editor_selector2, _real_column_types[column], *value);
    if (!data_editor)
      return;
    data_editor->set_title(base::strfmt("Edit Data for %s (%s)", _column_names[column].c_str(), logical_type.c_str()));
    data_editor->set_release_on_close(true);
    data_editor->signal_saved.connect(std::bind(&Recordset::set_field_value, this, row, column, data_editor));
    data_editor->show(true);
  }
  CATCH_AND_DISPATCH_EXCEPTION(false, "Open field editor")
}

class DataValueConv : public boost::static_visitor<sqlite::variant_t> {
public:
  DataValueConv() : _data(NULL), _length(0) {
  }
  DataValueConv(const char *data, size_t length) {
    set_data(data, length);
  }
  void set_data(const char *data, size_t length) {
    _data = data;
    _length = length;
  }

private:
  const char *_data;
  size_t _length;

public:
  result_type operator()(const sqlite::blob_ref_t &t) {
    sqlite::blob_ref_t val = sqlite::blob_ref_t(new sqlite::blob_t());
    val->resize(_length);
    memcpy(&(*val)[0], _data, _length);
    return val;
  }
  result_type operator()(const std::string &t) {
    return std::string(_data, _length);
  }
  template <typename T>
  result_type operator()(const T &t) {
    return sqlite::unknown_t();
  }
};

void Recordset::set_field_value(RowId row, ColumnId column, BinaryDataEditor *data_editor) {
  if (!data_editor)
    return;
  set_field_raw_data(row, column, data_editor->data(), data_editor->length(), data_editor->isJson());
}

void Recordset::set_field_raw_data(RowId row, ColumnId column, const char *data, size_t data_length,
                                   bool isJson /*= false*/) {
  DataValueConv data_value_conv(data, data_length);
  sqlite::variant_t valueString = std::string("");
  sqlite::variant_t value = boost::apply_visitor(data_value_conv, isJson ? valueString : _real_column_types[column]);
  if (sqlide::is_var_unknown(value))
    throw std::logic_error("Can't save value of this data type.");
  bec::NodeId node(row);
  set_field(node, column, value);
}

void Recordset::load_from_file(const bec::NodeId &node, ColumnId column, const std::string &file) {
  char *data;
  gsize length;
  GError *error = 0;

  if (!g_file_get_contents(file.c_str(), &data, &length, &error)) {
    mforms::Utilities::show_error("Cannot Load Field Value", error ? error->message : "Error loading file data", "OK");
    return;
  }

  try {
    set_field_raw_data(node[0], column, data, length);
  } catch (const std::exception &exc) {
    mforms::Utilities::show_error("Cannot Load Field Value", exc.what(), "OK", "", "");
  }
}

void Recordset::load_from_file(const bec::NodeId &node, ColumnId column) {
  mforms::FileChooser chooser(mforms::OpenFile);

  chooser.set_title("Load Field Value");

  if (chooser.run_modal())
    load_from_file(node, column, chooser.get_path());
}

class BlobCopier : public boost::static_visitor<void> {
public:
  BlobCopier() {
  }
  std::ostringstream os;

public:
  result_type operator()(const sqlite::blob_ref_t &v) {
    std::copy(v->begin(), v->end(), std::ostreambuf_iterator<char>(os));
  }
  result_type operator()(const std::string &v) {
    os << v;
  }
  template <typename T>
  result_type operator()(const T &) {
  }
};

bool Recordset::get_raw_field(const bec::NodeId &node, ColumnId column, std::string &data_ret) {
  base::RecMutexLock data_mutex(_data_mutex);

  sqlite::variant_t blob_value;
  sqlite::variant_t *value;

  if (sqlide::is_var_blob(_real_column_types[column])) {
    if (!_data_storage)
      return false;
    ssize_t rowid;
    if (!get_field_(node, _rowid_column, rowid))
      return false;
    std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
    _data_storage->fetch_blob_value(this, data_swap_db.get(), rowid, column, blob_value);
    value = &blob_value;
  } else {
    Cell cell;
    if (!get_cell(cell, node, column, false))
      return false;
    value = &(*cell);
  }

  BlobCopier copier;
  boost::apply_visitor(copier, *value);

  data_ret = copier.os.str();

  return true;
}

class DataValueDump : public boost::static_visitor<void> {
public:
  DataValueDump(const char *filename) : os(filename, std::ios_base::out | std::ios_base::binary) {
  }
  std::ofstream os;

public:
  result_type operator()(const sqlite::blob_ref_t &v) {
    std::copy(v->begin(), v->end(), std::ostreambuf_iterator<char>(os));
  }
  result_type operator()(const std::string &v) {
    os << v;
  }
  template <typename T>
  result_type operator()(const T &) {
  }
};

void Recordset::save_to_file(const bec::NodeId &node, ColumnId column, const std::string &file) {
  base::RecMutexLock data_mutex(_data_mutex);

  sqlite::variant_t blob_value;
  sqlite::variant_t *value;

  if (sqlide::is_var_blob(_real_column_types[column])) {
    if (!_data_storage)
      return;
    ssize_t rowid;
    if (!get_field_(node, _rowid_column, rowid))
      return;
    std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
    _data_storage->fetch_blob_value(this, data_swap_db.get(), rowid, column, blob_value);
    value = &blob_value;
  } else {
    Cell cell;
    if (!get_cell(cell, node, column, false))
      return;
    value = &(*cell);
  }

  DataValueDump data_value_dump(file.c_str());
  if (data_value_dump.os) {
    boost::apply_visitor(data_value_dump, *value);
  }
}

void Recordset::save_to_file(const bec::NodeId &node, ColumnId column) {
  mforms::FileChooser chooser(mforms::SaveFile);
  chooser.set_title("Save Field Value");
  chooser.set_extensions("Text files (*.txt)|*.txt|All Files (*.*)|*.*", "txt");

  if (chooser.run_modal()) {
    try {
      save_to_file(node, column, chooser.get_path());
    }
    CATCH_AND_DISPATCH_EXCEPTION(false, "Save field to file")
  }
}
