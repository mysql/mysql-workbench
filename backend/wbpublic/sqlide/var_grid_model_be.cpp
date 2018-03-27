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

#include "sqlide_generics_private.h"

#include "var_grid_model_be.h"
#include "base/string_utilities.h"
#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>
#include "glib/gstdio.h"
#include "base/boost_smart_ptr_helpers.h"

using namespace bec;
using namespace grt;
using namespace base;

//--------------------------------------------------------------------------------------------------

// sqlite supports up to 2000 columns (w/o need to recompile sources), see SQLITE_MAX_COLUMN on
// http://www.sqlite.org/limits.html
// but in fact we are restriced by more severe SQLITE_MAX_VARIABLE_NUMBER constant, which is 999 and which is used at
// max value when caching data
const int VarGridModel::DATA_SWAP_DB_TABLE_MAX_COL_COUNT = 999;

//--------------------------------------------------------------------------------------------------

class VarGridModel::IconForVal : public boost::static_visitor<IconId> {
public:
  IconForVal(bool treat_blobnull_as_blob) : _treat_blobnull_as_blob(treat_blobnull_as_blob) {
    IconManager *icon_man = IconManager::get_instance();
    _null_icon = icon_man->get_icon_id("field_overlay_null.png");
    _blob_icon = icon_man->get_icon_id("field_overlay_blob.png");
  }

private:
  IconId _null_icon;
  IconId _blob_icon;
  bool _treat_blobnull_as_blob;

public:
  template <typename T>
  result_type operator()(const T &t, const sqlite::null_t &v) const {
    return _null_icon;
  }
  result_type operator()(const sqlite::null_t &v) const {
    return _null_icon;
  }
  result_type operator()(const sqlite::blob_ref_t &t, const sqlite::null_t &v) const {
    return _treat_blobnull_as_blob ? _blob_icon : _null_icon;
  }
  template <typename V>
  result_type operator()(const sqlite::blob_ref_t &t, const V &v) const {
    return _blob_icon;
  }

  template <typename T, typename V>
  result_type operator()(const T &t, const V &v) const {
    return 0;
  }
};

//--------------------------------------------------------------------------------------------------

VarGridModel::VarGridModel()
  : _readonly(true),
    _row_count(0),
    _column_count(0),
    _data_frame_begin(0),
    _data_frame_end(0),
    _is_field_value_truncation_enabled(false),
    _edited_field_row(-1),
    _edited_field_col(-1) {
  {
    grt::DictRef options = DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
    _optimized_blob_fetching = (options.get_int("Recordset:OptimizeBlobFetching", 0) != 0);
  }
}

//--------------------------------------------------------------------------------------------------

VarGridModel::~VarGridModel() {
  _data_swap_db.reset();
  // clean temporary file to prevent crowding of files
  if (!_data_swap_db_path.empty())
    g_remove(_data_swap_db_path.c_str());
}

//--------------------------------------------------------------------------------------------------

void VarGridModel::reset() {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  _data_swap_db.reset();
  if (_data_swap_db_path.empty()) {
    _data_swap_db_path = GRTManager::get()->get_unique_tmp_subdir();
    _data_swap_db_path.resize(_data_swap_db_path.size() - 1); // remove trailing path separator
    _data_swap_db_path += ".db";

    std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();

    sqlite::execute(*data_swap_db, "create table `data` (`id` integer)", true);
    sqlite::execute(*data_swap_db, "create table `data_index` (`id` integer)", true);
    sqlite::execute(*data_swap_db, "create table `deleted_rows` (`id` integer)", true);
    sqlite::execute(*data_swap_db,
                    "create table `changes` (`id` integer primary key autoincrement, `record` integer, `action` "
                    "integer, `column` integer)",
                    true);
  }

  reinit(_data);
  reinit(_column_names);
  reinit(_column_types);
  reinit(_real_column_types);
  reinit(_column_flags);

  _column_count = 0;
  _row_count = 0;
  _data_frame_begin = 0;
  _data_frame_end = 0;

  _icon_for_val.reset(new IconForVal(_optimized_blob_fetching));
}

//--------------------------------------------------------------------------------------------------

int VarGridModel::floating_point_visible_scale() {
  grt::DictRef options = grt::DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
  return (int)options.get_int("Recordset:FloatingPointVisibleScale");
}

//--------------------------------------------------------------------------------------------------

std::shared_ptr<sqlite::connection> VarGridModel::data_swap_db() const {
  if (GRTManager::get()->in_main_thread())
    return (_data_swap_db) ? _data_swap_db : _data_swap_db = create_data_swap_db_connection();
  else
    return create_data_swap_db_connection();
}

//--------------------------------------------------------------------------------------------------

std::shared_ptr<sqlite::connection> VarGridModel::create_data_swap_db_connection() const {
  std::shared_ptr<sqlite::connection> data_swap_db;
  if (!_data_swap_db_path.empty()) {
    data_swap_db.reset(new sqlite::connection(_data_swap_db_path));
    sqlide::optimize_sqlite_connection_for_speed(data_swap_db.get());
  }
  return data_swap_db;
}

//--------------------------------------------------------------------------------------------------

int VarGridModel::refresh_ui() {
  if (GRTManager::get()->in_main_thread())
    refresh_ui_signal();
  else
    _refresh_connection = GRTManager::get()->run_once_when_idle(this, std::bind(&VarGridModel::refresh_ui, this));
  return 0;
}

//--------------------------------------------------------------------------------------------------

size_t VarGridModel::count() {
  return _row_count + (_readonly ? 0 : 1);
}

//--------------------------------------------------------------------------------------------------

class VarType {
public:
  typedef VarGridModel::ColumnType result_type;
  result_type operator()(const sqlite::blob_ref_t &) const {
    return VarGridModel::BlobType;
  }
  result_type operator()(int) const {
    return VarGridModel::NumericType;
  }
  result_type operator()(const std::int64_t &) const {
    return VarGridModel::NumericType;
  }
  result_type operator()(const long double &) const {
    return VarGridModel::FloatType;
  }
  template <typename T>
  result_type operator()(const T &v) const {
    return VarGridModel::StringType;
  }
};

//--------------------------------------------------------------------------------------------------

VarGridModel::ColumnType VarGridModel::get_column_type(ColumnId column) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  static VarType vt;
  return boost::apply_visitor(vt, _column_types[column]);
}

//--------------------------------------------------------------------------------------------------

VarGridModel::ColumnType VarGridModel::get_real_column_type(ColumnId column) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  static VarType vt;
  return boost::apply_visitor(vt, _real_column_types[column]);
}

bool VarGridModel::isGeometry(ColumnId column)
{
  base::RecMutexLock data_mutex WB_UNUSED (_data_mutex);
  return column < _dbColumnTypes.size() && _dbColumnTypes[column] == "GEOMETRY";
}

//--------------------------------------------------------------------------------------------------

std::string VarGridModel::get_column_caption(ColumnId column) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return _column_names.at(column);
}

VarGridModel::Cell VarGridModel::cell(RowId row, ColumnId column) {
  if (row >= _row_count)
    return _data.end();

  // cache rows if needed
  if ((_data_frame_begin > row) || (_data_frame_end <= row) || ((_data_frame_end == _data_frame_begin) && _row_count))
    cache_data_frame(row, false);

  // translate to absolute cell address
  RowId cell_index = (row - _data_frame_begin) * _column_count + column;
  return _data.begin() + cell_index;
}

bool VarGridModel::get_cell(VarGridModel::Cell &cell, const NodeId &node, ColumnId column, bool allow_new_row) {
  if (!node.is_valid())
    return false;

  RowId row = node[0];

  if ((row > _row_count) || (column >= _column_count) || (!allow_new_row && (_row_count == row)))
    return false;

  cell = this->cell(row, column);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::is_field_null(const NodeId &node, ColumnId column) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);

  // returns true for out of the range addresses
  Cell cell;
  if (get_cell(cell, node, column, false)) {
    if (_optimized_blob_fetching && sqlide::is_var_blob(_real_column_types[column]))
      return false;
    else
      return sqlide::is_var_null(*cell);
  } else {
    return true;
  }
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::set_field_null(const bec::NodeId &node, ColumnId column) {
  return is_field_null(node, column) ? true : set_field(node, column, sqlite::null_t());
}

//--------------------------------------------------------------------------------------------------

IconId VarGridModel::get_field_icon(const NodeId &node, ColumnId column, IconSize size) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);

  Cell cell;
  static const sqlite::variant_t null_value((sqlite::null_t()));
  if (((ssize_t)column < 0) || (column + 1 >= _column_types.size()))
    return 0;
  const sqlite::variant_t &var = get_cell(cell, node, column, false) ? *cell : null_value;
  return boost::apply_visitor(*_icon_for_val, _column_types[column], var);
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field(const NodeId &node, ColumnId column, std::string &value) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return get_field_(node, column, value);
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_(const NodeId &node, ColumnId column, std::string &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res)
    value = boost::apply_visitor(_var_to_str, *cell);
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_repr(const NodeId &node, ColumnId column, std::string &value) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return get_field_repr_(node, column, value);
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_repr_no_truncate(const bec::NodeId &node, ColumnId column, std::string &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res)
    value = boost::apply_visitor(_var_to_str_repr, *cell);
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_repr_(const NodeId &node, ColumnId column, std::string &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res) {
    if (_is_field_value_truncation_enabled) {
      size_t row = node[0];
      _var_to_str_repr.is_truncation_enabled = (row != _edited_field_row) || (column != _edited_field_col);
    }
    value = boost::apply_visitor(_var_to_str_repr, *cell);
  }
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field(const NodeId &node, ColumnId column, sqlite::variant_t &value) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return get_field_(node, column, value);
}

bool VarGridModel::get_field(const NodeId &node, ColumnId column, bool &value) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return get_field_(node, column, value);
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_(const NodeId &node, ColumnId column, sqlite::variant_t &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res)
    value = *cell;
  return res;
}

bool VarGridModel::get_field_(const NodeId &node, ColumnId column, bool &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res)
    value = (ssize_t)boost::apply_visitor(_var_to_bool, *cell);
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field(const NodeId &node, ColumnId column, ssize_t &value) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return get_field_(node, column, value);
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_(const NodeId &node, ColumnId column, ssize_t &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res)
    value = (ssize_t)boost::apply_visitor(_var_to_int, *cell);
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field(const NodeId &node, ColumnId column, double &value) {
  base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);
  return get_field_(node, column, value);
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_(const NodeId &node, ColumnId column, double &value) {
  Cell cell;
  bool res = get_cell(cell, node, column, false);
  if (res)
    value = (double)boost::apply_visitor(_var_to_long_double, *cell);
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  std::string val;
  bool res = get_field(node, column, val);
  if (res)
    value = grt::StringRef(val);
  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::set_field(const NodeId &node, ColumnId column, const sqlite::variant_t &value) {
  bool res = false;

  {
    base::RecMutexLock data_mutex WB_UNUSED(_data_mutex);

    Cell cell;
    res = get_cell(cell, node, column, true);
    if (res) {
      bool is_blob_column = sqlide::is_var_blob(_real_column_types[column]);
      if (!_optimized_blob_fetching || !is_blob_column) {
        static const sqlide::VarEq var_eq;
        if (!is_blob_column)
          res = !boost::apply_visitor(var_eq, value, *cell);
        if (res)
          *cell = value;
      }
    }
  }

  if (res)
    after_set_field(node, column, value);

  return res;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  return set_field(node, column, sqlite::variant_t(value));
}

bool VarGridModel::set_field(const NodeId &node, ColumnId column, bool value) {
  return set_field(node, column, sqlite::variant_t((bool)value));
}
//--------------------------------------------------------------------------------------------------

bool VarGridModel::set_field(const NodeId &node, ColumnId column, double value) {
  return set_field(node, column, sqlite::variant_t((long double)value));
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  return set_field(node, column, sqlite::variant_t((std::int64_t)value));
}

//--------------------------------------------------------------------------------------------------

void VarGridModel::add_column(const std::string &name, const sqlite::variant_t &type) {
  _column_names.push_back(name);
  _column_types.push_back(type);
  _real_column_types.push_back(type);
  ++_column_count;
}

//--------------------------------------------------------------------------------------------------

void VarGridModel::cache_data_frame(RowId center_row, bool force_reload) {
  static const RowId half_row_count = 500; //! load from options
  RowId row_count = half_row_count * 2;

  // center_row of -1 means only to forcibly reload current data frame
  if (-1 != (int)center_row) {
    RowId starting_row = (center_row < half_row_count) ? 0 : (center_row - half_row_count);

    // adjust range borders to comply with row count & cache frame size
    // e.g. shift back and/or resize requested cache frame
    if (starting_row + row_count > _row_count) {
      if (row_count < _row_count) {
        starting_row = _row_count - row_count;
      } else {
        starting_row = 0;
        row_count = _row_count;
      }
    }

    if (!force_reload && (_data_frame_begin == starting_row) && (_data_frame_begin != _data_frame_end) &&
        (_data_frame_end - _data_frame_begin == row_count)) {
      return;
    }

    _data_frame_begin = starting_row;
    _data_frame_end = starting_row + row_count;
  } else {
    row_count = _data_frame_end - _data_frame_begin;
  }

  _data.clear();

  // load data
  {
    std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db();
    const size_t partition_count = data_swap_db_partition_count();

    std::list<std::shared_ptr<sqlite::query> > data_queries(partition_count);
    prepare_partition_queries(
      data_swap_db.get(),
      "select d.* from `data%s` d inner join `data_index` di on (di.`id`=d.`id`) order by di.`rowid` limit ? offset ?",
      data_queries);
    std::list<sqlite::variant_t> bind_vars;
    bind_vars.push_back((int)row_count);
    bind_vars.push_back((int)_data_frame_begin);
    std::vector<std::shared_ptr<sqlite::result> > data_results(data_queries.size());
    if (emit_partition_queries(data_swap_db.get(), data_queries, data_results, bind_vars)) {
      bool next_row_exists = true;

      std::vector<bool> blob_columns(_column_count);
      for (ColumnId col = 0; _column_count > col; ++col)
        blob_columns[col] = sqlide::is_var_blob(_real_column_types[col]);

      _data.reserve(row_count * _column_count);
      do {
        for (size_t partition = 0; partition < partition_count; ++partition) {
          std::shared_ptr<sqlite::result> &data_rs = data_results[partition];
          for (ColumnId col_begin = partition * DATA_SWAP_DB_TABLE_MAX_COL_COUNT, col = col_begin,
                        col_end = std::min<ColumnId>(_column_count, (partition + 1) * DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
               col < col_end; ++col) {
            sqlite::variant_t v;
            if (_optimized_blob_fetching && blob_columns[col]) {
              v = sqlite::null_t();
            } else {
              ColumnId partition_column = col - col_begin;
              v = data_rs->get_variant((int)partition_column);
              v = boost::apply_visitor(_var_cast, _column_types[col], v);
            }
            _data.push_back(v);
          }
        }
        for (auto &data_rs : data_results)
          next_row_exists = data_rs->next_row();
      } while (next_row_exists);
    }
  }
}

//--------------------------------------------------------------------------------------------------

size_t VarGridModel::data_swap_db_partition_count() const {
  return data_swap_db_partition_count(_column_count);
}

//--------------------------------------------------------------------------------------------------

size_t VarGridModel::data_swap_db_partition_count(ColumnId column_count) {
  std::div_t d = std::div((int)column_count, DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
  return d.quot + (size_t)(d.rem > 0);
}

//--------------------------------------------------------------------------------------------------

std::string VarGridModel::data_swap_db_partition_suffix(size_t partition_index) {
  return (partition_index > 0) ? strfmt("_%u", (unsigned int)partition_index) : std::string("");
}

//--------------------------------------------------------------------------------------------------

size_t VarGridModel::data_swap_db_column_partition(ColumnId column) {
  std::div_t d = std::div((int)column, DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
  return d.quot;
}

//--------------------------------------------------------------------------------------------------

bec::ListModel::ColumnId VarGridModel::translate_data_swap_db_column(ListModel::ColumnId column, size_t *partition) {
  std::div_t d = std::div((int)column, DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
  if (partition)
    *partition = d.quot;
  return d.rem;
}

//--------------------------------------------------------------------------------------------------

void VarGridModel::prepare_partition_queries(sqlite::connection *data_swap_db, const std::string &query_text_template,
                                             std::list<std::shared_ptr<sqlite::query> > &queries) {
  size_t partition = 0;
  for (auto &query : queries) {
    std::string partition_suffix = data_swap_db_partition_suffix(partition);
    query.reset(new sqlite::query(*data_swap_db, strfmt(query_text_template.c_str(), partition_suffix.c_str())));
    ++partition;
  }
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::emit_partition_queries(sqlite::connection *data_swap_db,
                                          std::list<std::shared_ptr<sqlite::query> > &queries,
                                          std::vector<std::shared_ptr<sqlite::result> > &results,
                                          const std::list<sqlite::variant_t> &bind_vars) {
  //  bool no_results_returned= false;
  size_t partition = 0;
  for (auto &query : queries) {
    query->clear();
    sqlide::BindSqlCommandVar bind_sql_command_var(query.get());
    for (const auto &var : bind_vars)
      boost::apply_visitor(bind_sql_command_var, var);

    if (!query->emit()) {
      //      no_results_returned= true;
      return false;
    }

    results[partition] = BoostHelper::convertPointer(query->get_result());
    ++partition;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void VarGridModel::emit_partition_commands(sqlite::connection *data_swap_db, size_t partition_count,
                                           const std::string &command_text_template,
                                           const std::list<sqlite::variant_t> &bind_vars) {
  for (std::size_t partition = 0; partition < partition_count; ++partition) {
    std::string partition_suffix = data_swap_db_partition_suffix(partition);
    sqlite::command command(*data_swap_db, strfmt(command_text_template.c_str(), partition_suffix.c_str()));
    sqlide::BindSqlCommandVar bind_sql_command_var(&command);

    for (const auto &var : bind_vars)
      boost::apply_visitor(bind_sql_command_var, var);
    command.emit();
  }
}

//--------------------------------------------------------------------------------------------------

void VarGridModel::set_edited_field(RowId row_index, ColumnId col_index) {
  _edited_field_row = row_index;
  _edited_field_col = col_index;
}

//--------------------------------------------------------------------------------------------------

bool VarGridModel::is_field_value_truncation_enabled(bool val) {
  _is_field_value_truncation_enabled = val;
  if (_is_field_value_truncation_enabled) {
    grt::DictRef options = grt::DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
    ssize_t field_value_truncation_threshold = options.get_int("Recordset:FieldValueTruncationThreshold", 256);
    if (field_value_truncation_threshold < 0)
      _var_to_str_repr.is_truncation_enabled = _is_field_value_truncation_enabled = false;
    else
      _var_to_str_repr.truncation_threshold = field_value_truncation_threshold;
  } else
    _var_to_str_repr.is_truncation_enabled = _is_field_value_truncation_enabled;

  return _is_field_value_truncation_enabled;
}

//--------------------------------------------------------------------------------------------------
