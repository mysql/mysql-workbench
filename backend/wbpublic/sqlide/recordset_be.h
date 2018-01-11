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

#pragma once

#include "wbpublic_public_interface.h"
#include "sqlide/sqlide_generics.h"
#include "sqlide/var_grid_model_be.h"
#include "grt/action_list.h"
#include <map>
#include <set>
#include <list>

class Recordset_data_storage;
class BinaryDataEditor;

namespace mforms {
  class Form;
  class ContextMenu;
  class ToolBar;
  class ToolBarItem;
};

struct WBPUBLICBACKEND_PUBLIC_FUNC Recordset_storage_info {
  std::string name;
  std::string extension;
  std::string description;
  // "label1":SYMBOL1;label2:SYMBOL2;label3:SYMBOL3
  std::list<std::pair<std::string, std::string> > arguments;
};

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset : public VarGridModel {
public:
  typedef std::shared_ptr<Recordset> Ref;
  typedef std::weak_ptr<Recordset> Ptr;
  static Ref create();
  static Ref create(GrtThreadedTask::Ref parent_task);
  virtual ~Recordset();

protected:
  Recordset();
  Recordset(GrtThreadedTask::Ref parent_task);

public:
  bool can_close();
  bool can_close(bool interactive);
  bool close();
  boost::signals2::signal<void(Ptr)> on_close;

public:
  typedef std::shared_ptr<Recordset_data_storage> Recordset_data_storage_Ref;
  typedef std::weak_ptr<Recordset_data_storage> Recordset_data_storage_Ptr;
  friend class Recordset_data_storage;

public:
  long key() const {
    return _id;
  }

  class WBPUBLICBACKEND_PUBLIC_FUNC ClientData {
  public:
    virtual ~ClientData();
  };

  void set_client_data(ClientData *cdata) {
    _client_data = cdata;
  }
  ClientData *client_data() {
    return _client_data;
  }

public:
  bool reset(bool rethrow);
  virtual void reset();
  virtual void refresh();
  boost::signals2::signal<void()> data_edited_signal;

private:
  bool reset(Recordset_data_storage_Ptr data_storage_ptr, bool rethrow);
  void data_edited();

public:
  RowId real_row_count() const;

private:
  void recalc_row_count(sqlite::connection *data_swap_db);

private:
  size_t _real_row_count;

public:
  const Column_names *column_names() const {
    return &_column_names;
  }
  virtual size_t get_column_count() const {
    return (int)(_column_count - _aux_column_count);
  }
  size_t aux_column_count() const {
    return _aux_column_count;
  }

protected:
  size_t _aux_column_count;
  ColumnId _rowid_column;

public:
  RowId min_new_rowid() const {
    return _min_new_rowid;
  }

protected:
  RowId _min_new_rowid;
  RowId _next_new_rowid;

private:
  static std::string _add_change_record_statement;

public:
  virtual void after_set_field(const bec::NodeId &node, ColumnId column, const sqlite::variant_t &value);
  virtual bool delete_node(const bec::NodeId &node);
  virtual bool delete_nodes(std::vector<bec::NodeId> &nodes);

private:
  virtual Cell cell(RowId row, ColumnId column);
  void mark_dirty(RowId row, ColumnId column, const sqlite::variant_t &new_value);

public:
  Recordset_data_storage_Ref data_storage() {
    return _data_storage;
  }
  void data_storage(const Recordset_data_storage_Ref &data_storage) {
    _data_storage = data_storage;
  }

protected:
  Recordset_data_storage_Ref _data_storage;

public:
  std::function<void()> apply_changes_cb;
  // force UI to save any ongoing edits by the user
  std::function<void()> flush_ui_changes_cb;

public:
  bool apply_changes_and_gather_messages(std::string &messages);
  void rollback_and_gather_messages(std::string &messages);

  void apply_changes_();
  grt::StringRef do_apply_changes(Ptr self_ptr, Recordset_data_storage_Ptr data_storage_ptr, bool skip_commit);
  bool has_pending_changes();
  void pending_changes(int &upd_count, int &ins_count, int &del_count) const;
  void rollback();
  void apply_changes();

private:
  int on_apply_changes_finished();
  void apply_changes_(Recordset_data_storage_Ptr data_storage_ptr);

public:
  bool limit_rows();
  void limit_rows(bool value);
  void toggle_limit_rows();
  int limit_rows_count();
  void limit_rows_count(int value);
  bool limit_rows_applicable();
  void scroll_rows_frame_forward();
  void scroll_rows_frame_backward();

public:
  mforms::ContextMenu *get_context_menu();

  void update_selection_for_menu(const std::vector<int> &rows, int clicked_column);
  std::function<void(mforms::ContextMenu *, const std::vector<int> &, int)> update_selection_for_menu_extra;

  std::vector<int> selected_rows() {
    return _selected_rows;
  }
  int selected_column() {
    return _selected_column;
  }

private:
  std::vector<int> _selected_rows;
  int _selected_column;

  void activate_menu_item(const std::string &action, const std::vector<int> &rows, int clicked_column);

public:
  void copy_rows_to_clipboard(const std::vector<int> &indeces, std::string sep = ", ", bool quoted = true,
                              bool with_header = false);
  void copy_field_to_clipboard(int row, ColumnId column, bool quoted = true);

  void paste_rows_from_clipboard(ssize_t dest_row);
  void showPointInBrowser(const bec::NodeId &node, ColumnId column);
  std::vector<Recordset_storage_info> data_storages_for_export();
  Recordset_data_storage_Ref data_storage_for_export(const std::string &format_name);

protected:
  Recordset_data_storage_Ref _data_storage_for_export;
  typedef std::map<std::string, std::string> Data_storages_for_export;
  Data_storages_for_export _data_storages_for_export;

  void load_from_file(const bec::NodeId &node, ColumnId column);
  void save_to_file(const bec::NodeId &node, ColumnId column);

public:
  void load_from_file(const bec::NodeId &node, ColumnId column, const std::string &file);
  void save_to_file(const bec::NodeId &node, ColumnId column, const std::string &file);

  bool get_raw_field(const bec::NodeId &node, ColumnId column, std::string &data_ret);

public:
  virtual void sort_by(ColumnId column, int direction, bool retaining);
  virtual SortColumns sort_columns() const {
    return _sort_columns;
  }

private:
  SortColumns _sort_columns; // column:direction(asc/desc)

public:
  bool has_column_filters() const;
  bool has_column_filter(ColumnId column) const;
  std::string get_column_filter_expr(ColumnId column) const;
  void set_column_filter(ColumnId column, const std::string &filter_expr);
  void reset_column_filter(ColumnId column);
  void reset_column_filters();
  size_t column_filter_icon_id() const;

private:
  typedef std::map<ColumnId, std::string> Column_filter_expr_map;
  Column_filter_expr_map _column_filter_expr_map; // column:filter_expr

  void search_activated(mforms::ToolBarItem *item);

public:
  const std::string &data_search_string() const;
  void set_data_search_string(const std::string &value);
  void reset_data_search_string();
  void setPreserveRowFilter(bool value) {
    _preserveRowFilters = value;
  }

private:
  std::string _data_search_string;
  bool _preserveRowFilters;

private:
  void rebuild_data_index(sqlite::connection *data_swap_db, bool do_cache_data_frame, bool do_refresh_ui);

public:
  void caption(const std::string &val) {
    _caption = val;
  }
  std::string caption();
  void set_inserts_editor(bool flag) {
    _inserts_editor = flag;
  }
  bool inserts_editor() {
    return _inserts_editor;
  }

  std::string generator_query() const {
    return _generator_query;
  }
  void generator_query(const std::string &query) {
    _generator_query = query;
  }

  std::string status_text();
  std::string status_text_trailer;

private:
  bool _inserts_editor;
  std::string _caption;
  std::string _generator_query;
  long _id;
  ClientData *_client_data;
  mforms::ToolBar *_toolbar;

public:
  GrtThreadedTask::Ref task;

public:
  mforms::ContextMenu *_context_menu;

public:
  ::ActionList &action_list();

private:
  ::ActionList _action_list;

public:
  mforms::ToolBar *get_toolbar();
  void rebuild_toolbar();

private:
  void register_default_actions();

public:
  void open_field_data_editor(RowId row, ColumnId column, const std::string &logical_type);

protected:
  void set_field_value(RowId row, ColumnId column, BinaryDataEditor *data_editor);
  void set_field_raw_data(RowId row, ColumnId column, const char *data, size_t data_length, bool isJson = false);

public:
  const std::string &getFont() const { return _font; }
  float getFontSize() const { return _size; }
  void setFont(const std::string &font, float size) { _font = font; _size = size; }

private:
  std::string _font;
  float _size;
};
