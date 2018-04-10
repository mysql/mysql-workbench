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

#ifndef _RECORDSET_TABLE_INSERTS_STORAGE_BE_H_
#define _RECORDSET_TABLE_INSERTS_STORAGE_BE_H_

#include "wbpublic_public_interface.h"
#include "sqlide/recordset_sqlite_storage.h"
#include "grts/structs.db.h"
#include <vector>

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset_table_inserts_storage : public Recordset_sqlite_storage {
public:
  typedef std::shared_ptr<Recordset_table_inserts_storage> Ref;
  static Ref create() {
    return create_with_path(bec::GRTManager::get()->get_db_file_path());
  }
  static Ref create_with_path(const std::string &path) {
    return Ref(new Recordset_table_inserts_storage(path));
  }
  virtual ~Recordset_table_inserts_storage();

protected:
  Recordset_table_inserts_storage(const std::string &path);

protected:
  virtual void do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db, bool skip_commit);
  virtual void do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db);
  void do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                           sqlite::variant_t &blob_value);

private:
  Recordset::Column_names _mapped_colnames;
  std::string _mapped_table_name;

protected:
  void generate_sql_script(const Recordset *recordset, sqlite::connection *data_swap_db, Sql_script &sql_script,
                           bool is_update_script, bool binaryAsString = false);

public:
  void table(const db_TableRef table) {
    _table = table;
  }
  db_TableRef table() const {
    return _table;
  }

protected:
  db_TableRef _table;
};

#endif /* _RECORDSET_TABLE_INSERTS_STORAGE_BE_H_ */
