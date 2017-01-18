/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _RECORDSET_SQLITE_STORAGE_BE_H_
#define _RECORDSET_SQLITE_STORAGE_BE_H_

#include "wbpublic_public_interface.h"
#include "sqlide/recordset_sql_storage.h"

namespace sqlite {
  struct connection;
}

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset_sqlite_storage : public Recordset_sql_storage {
public:
  typedef std::shared_ptr<Recordset_sqlite_storage> Ref;
  static Ref create() {
    return Ref(new Recordset_sqlite_storage());
  }
  virtual ~Recordset_sqlite_storage();

protected:
  Recordset_sqlite_storage();

protected:
  virtual void do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                   sqlite::variant_t &blob_value);

protected:
  virtual void run_sql_script(const Sql_script &sql_script, bool skip_commit);

protected:
  std::string decorated_sql_query(Recordset::Column_names &column_names);

public:
  void db_path(const std::string &db_path) {
    _db_path = db_path;
  }
  const std::string &db_path() const {
    return _db_path;
  }

private:
  std::string _db_path;
};

#endif /* _RECORDSET_SQLITE_STORAGE_BE_H_ */
