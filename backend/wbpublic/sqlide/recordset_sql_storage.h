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

#ifndef _RECORDSET_SQL_STORAGE_BE_H_
#define _RECORDSET_SQL_STORAGE_BE_H_

#include "wbpublic_public_interface.h"
#include "sqlide/recordset_data_storage.h"
#include "grtsqlparser/sql_inserts_loader.h"
#include "grts/structs.db.mgmt.h"
#include <vector>

class WBPUBLICBACKEND_PUBLIC_FUNC Sql_script {
public:
  typedef std::list<std::string> Statements;
  typedef std::list<sqlite::variant_t> Statement_bindings;
  typedef std::list<Statement_bindings> Statements_bindings;
  Statements statements;
  Statements_bindings statements_bindings;
  void reset() {
    statements.clear();
    statements_bindings.clear();
  }
};

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset_sql_storage : public Recordset_data_storage {
public:
  typedef std::shared_ptr<Recordset_sql_storage> Ref;
  static Ref create() {
    return Ref(new Recordset_sql_storage());
  }
  virtual ~Recordset_sql_storage();

protected:
  Recordset_sql_storage();

protected:
  virtual void fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                sqlite::variant_t &blob_value);

protected:
  virtual void do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db, bool skip_commit);
  virtual void do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                   sqlite::variant_t &blob_value);

public:
  static std::string statements_as_sql_script(const Sql_script::Statements &sql_statements);

protected:
  virtual void generate_sql_script(const Recordset *recordset, sqlite::connection *data_swap_db, Sql_script &sql_script,
                                   bool is_update_script, bool binaryAsString = false);
  virtual void generate_inserts(const Recordset *recordset, sqlite::connection *data_swap_db, Sql_script &sql_script);
  virtual void run_sql_script(const Sql_script &sql_script, bool skip_commit) {
  }
  virtual void init_variant_quoter(sqlide::QuoteVar &qv) const;

public:
  void schema_name(const std::string &schema_name) {
    _schema_name = schema_name;
  }
  std::string schema_name() const {
    return _schema_name;
  }

  void table_name(const std::string &table_name) {
    _table_name = table_name;
  }
  std::string table_name() const {
    return _table_name;
  }

  std::string full_table_name() const;

  void additional_clauses(const std::string &value) {
    _additional_clauses = value;
  }
  std::string additional_clauses() const {
    return _additional_clauses;
  }

  void sql_query(const std::string &sql_query) {
    _sql_query = sql_query;
  }
  std::string sql_query() const {
    return _sql_query;
  }

  void sql_script(const std::string &val) {
    _sql_script = val;
  }
  const std::string &sql_script() const {
    return _sql_script;
  }

  void affective_columns(const Sql_inserts_loader::Strings &val) {
    _affective_columns = val;
  }
  const Sql_inserts_loader::Strings &affective_columns() const {
    return _affective_columns;
  }

  virtual ColumnId aux_column_count() {
    return _pkey_columns.size();
  }

protected:
  std::string _table_name;
  std::string _schema_name;
  std::string _additional_clauses;
  std::string _sql_query;
  std::string _sql_script; // for storing result of serialize
  std::vector<ColumnId> _pkey_columns;
  sqlide::VarCast _var_cast;
  Sql_inserts_loader::Strings _affective_columns; // used to filter irrelevant fields when loading from custom (not
                                                  // validated) sql script, also affects column order

public:
  void sql_script_substitute(const Sql_script &val) {
    _sql_script_substitute = val;
  }
  const Sql_script &sql_script_substitute() const {
    return _sql_script_substitute;
  }
  void is_sql_script_substitute_enabled(bool val) {
    _is_sql_script_substitute_enabled = val;
  }
  bool is_sql_script_substitute_enabled() const {
    return _is_sql_script_substitute_enabled;
  }

public:
  void init_sql_script_substitute(const Recordset::Ptr &recordset, bool is_update_script);
  void omit_schema_qualifier(bool flag);

private:
  void do_init_sql_script_substitute(const Recordset *recordset, sqlite::connection *data_swap_db,
                                     bool is_update_script);

private:
  Sql_script _sql_script_substitute; // if (_is_sql_script_substitute_enabled) use this value instead of generating sql
                                     // script with generate_sql_script
  bool _is_sql_script_substitute_enabled;
  bool _omit_schema_qualifier;

private:
  typedef std::map<std::string, int> Fields_order;
  Fields_order _fields_order;

  void load_insert_statement(const std::string &sql, const std::pair<std::string, std::string> &schema_table,
                             const Sql_inserts_loader::Strings &fields_names,
                             const Sql_inserts_loader::Strings &fields_values, const std::vector<bool> &null_fields,
                             Recordset::Column_names *column_names, Var_list *var_list);

public:
  db_mgmt_RdbmsRef rdbms() {
    return _rdbms;
  }
  void rdbms(db_mgmt_RdbmsRef rdbms) {
    _rdbms = rdbms;
  }

protected:
  db_mgmt_RdbmsRef _rdbms;

public:
  typedef boost::signals2::signal<int(long long, const std::string &, const std::string &)> Error_cb;
  typedef boost::signals2::signal<int(float)> Batch_exec_progress_cb;
  typedef boost::signals2::signal<int(long, long)> Batch_exec_stat_cb;
  Error_cb on_sql_script_run_error;
  Batch_exec_progress_cb on_sql_script_run_progress;
  Batch_exec_stat_cb on_sql_script_run_statistics;

protected:
  void get_pkey_predicate_for_data_cache_rowid(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid,
                                               std::string &pkey_predicate);

public:
  bool binding_blobs() const {
    return _binding_blobs;
  }
  void binding_blobs(bool val) {
    _binding_blobs = val;
  }

private:
  bool _binding_blobs;
};

namespace sqlite {
  struct result;
}

class PrimaryKeyPredicate {
  const Recordset::Column_types *_column_types;
  const Recordset::Column_names *_column_names;
  const std::vector<ColumnId> *_pkey_columns;
  sqlide::QuoteVar *_qv;

public:
  PrimaryKeyPredicate(const Recordset::Column_types *column_types, const Recordset::Column_names *column_names,
                      const std::vector<ColumnId> *pkey_columns, sqlide::QuoteVar *qv);
  std::string operator()(std::vector<std::shared_ptr<sqlite::result> > &data_row_results);
};

#endif /* _RECORDSET_SQL_STORAGE_BE_H_ */
