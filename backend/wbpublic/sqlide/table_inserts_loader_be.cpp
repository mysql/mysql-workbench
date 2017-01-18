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

#include "table_inserts_loader_be.h"
#include "recordset_table_inserts_storage.h"
#include "recordset_be.h"

using namespace grt;

TableInsertsLoader::TableInsertsLoader() {
}

void TableInsertsLoader::process_table(db_TableRef table, const std::string &inserts_script) //!
{
  if (!table.is_valid() || inserts_script.empty())
    return;

  Recordset_sql_storage::Ref input_storage = Recordset_sql_storage::create();
  input_storage->sql_script(inserts_script);
  input_storage->schema_name(table->owner()->name());
  input_storage->table_name(table->name());
  {
    Sql_inserts_loader::Strings affective_columns;
    affective_columns.reserve(table->columns().count());
    GRTLIST_FOREACH(db_Column, table->columns(), col)
    affective_columns.push_back((*col)->name());
    input_storage->affective_columns(affective_columns);
  }

  Recordset::Ref rs = Recordset::create();
  rs->data_storage(input_storage);
  rs->reset();

  Recordset_table_inserts_storage::Ref output_storage = Recordset_table_inserts_storage::create();
  output_storage->table(table);
  // provoke creation of underlying table
  {
    Recordset::Ref rs = Recordset::create();
    output_storage->unserialize(rs);
  }
  output_storage->serialize(rs);
}
