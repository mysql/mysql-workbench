/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DB_SQL_EDITOR_HISTORY_BE_H_
#define _DB_SQL_EDITOR_HISTORY_BE_H_

#include "workbench/wb_backend_public_interface.h"
#include "sqlide/var_grid_model_be.h"
#include <time.h>
#include "mforms/menu.h"

class MYSQLWBBACKEND_PUBLIC_FUNC DbSqlEditorHistory {
public:
  typedef std::shared_ptr<DbSqlEditorHistory> Ref;
  static Ref create() {
    return Ref(new DbSqlEditorHistory());
  }
  virtual ~DbSqlEditorHistory();

protected:
  DbSqlEditorHistory();

public:
  void reset();
  void add_entry(const std::list<std::string> &statements);
  int current_entry() {
    return _current_entry_index;
  }
  void current_entry(int index);
  std::string restore_sql_from_history(int entry_index, std::list<int> &detail_indexes);

protected:
  int _current_entry_index;

public:
  void load();

public:
  class EntriesModel;
  class DetailsModel;

public:
  class DetailsModel : public VarGridModel {
  public:
    friend class DbSqlEditorHistory;
    typedef std::shared_ptr<DetailsModel> Ref;
    static Ref create() {
      return Ref(new DetailsModel());
    }

  protected:
    DetailsModel();

  public:
    void add_entries(const std::list<std::string> &statements);
    virtual void refresh() {
      refresh_ui();
    }
    mforms::Menu *get_context_menu() {
      return &_context_menu;
    }

    virtual void reset();

    void save();
    void load(const std::string &storage_file_path);

  protected:
    int _last_loaded_row; // required to skip duplication of existing entries when dumping contents

  public:
    std::tm datestamp() const {
      return _datestamp;
    }
    void datestamp(const std::tm &val) {
      _datestamp = val;
    }

  protected:
    std::string storage_file_path() const;
    std::tm _datestamp; // raw datestamp for locale independent storage file name

  private:
    grt::StringRef _last_timestamp;
    grt::StringRef _last_statement;
    mforms::Menu _context_menu;
  };

public:
  class EntriesModel : public VarGridModel {
  private:
    bool _ui_usage;

  public:
    friend class DbSqlEditorHistory;

    typedef std::shared_ptr<EntriesModel> Ref;
    static Ref create(DbSqlEditorHistory *owner) {
      return Ref(new EntriesModel(owner));
    }

  protected:
    EntriesModel(DbSqlEditorHistory *owner);

    DbSqlEditorHistory *_owner;

    void add_statements(const std::list<std::string> &statements);

  public:
    bool insert_entry(const std::tm &t);
    void delete_all_entries();
    void delete_entries(const std::vector<std::size_t> &rows);
    void set_ui_usage(bool value) {
      _ui_usage = value;
    }
    bool get_ui_usage() {
      return _ui_usage;
    }

    std::string entry_path(std::size_t index);
    std::tm entry_date(std::size_t index);

    virtual void reset();
    void load();

    virtual bool activate_popup_item_for_nodes(const std::string &action, const std::vector<bec::NodeId> &orig_nodes);
    virtual bec::MenuItemList get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes);
  };

public:
  EntriesModel::Ref entries_model() {
    return _entries_model;
  }
  DetailsModel::Ref details_model() {
    return _details_model;
  }
  DetailsModel::Ref write_only_details_model() {
    return _write_only_details_model;
  }

protected:
  EntriesModel::Ref _entries_model;
  DetailsModel::Ref _details_model;
  DetailsModel::Ref _write_only_details_model;

  void update_timestamp(std::tm timestamp);
};

#endif /* _DB_SQL_EDITOR_HISTORY_BE_H_ */
