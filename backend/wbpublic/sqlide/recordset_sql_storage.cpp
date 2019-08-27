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

#include <sqlite/query.hpp>

#include "recordset_sql_storage.h"
#include "recordset_be.h"
#include "grtsqlparser/sql_facade.h"
#include "base/string_utilities.h"
#include "base/sqlstring.h"
#include "base/boost_smart_ptr_helpers.h"
#include <algorithm>
#include <sstream>
#include "mforms/jsonview.h"

using namespace bec;
using namespace grt;
using namespace base;

//------------------------------------------------------------------------------

PrimaryKeyPredicate::PrimaryKeyPredicate(const Recordset::Column_types *column_types,
                                         const Recordset::Column_names *column_names,
                                         const std::vector<ColumnId> *pkey_columns, sqlide::QuoteVar *qv)
  : _column_types(column_types), _column_names(column_names), _pkey_columns(pkey_columns), _qv(qv) {
}

std::string PrimaryKeyPredicate::operator()(std::vector<std::shared_ptr<sqlite::result> > &data_row_results) {
  std::string predicate;
  sqlite::variant_t v;

  for (auto col : *_pkey_columns) {
    if (!predicate.empty())
      predicate += " and ";

    size_t partition;
    ColumnId partition_column = Recordset::translate_data_swap_db_column(col, &partition);
    std::shared_ptr<sqlite::result> &data_row_rs = data_row_results[partition];

    v = data_row_rs->get_variant((int)partition_column);
    predicate += "(`" + (*_column_names)[col] + "`";
    std::string value = boost::apply_visitor(*_qv, (*_column_types)[col], v);
    predicate += (value == "NULL" ? " IS NULL" : " = " + value) + ")";
  }

  return predicate;
}

//------------------------------------------------------------------------------

class JsonTypeFinder : public boost::static_visitor<bool> {
public:
  result_type operator()(const sqlite::unknown_t &, const std::string &text) const {
    bool ret = false;
    rapidjson::Value value;
    rapidjson::Document d;
    d.Parse(text.c_str());
    if (!d.HasParseError()) {
      value.CopyFrom(d, d.GetAllocator());
      ret = true;
    }
    return ret;
  }

  template <typename T, typename V>
  result_type operator()(const T &t, const V &v) const {
    return false;
  }
};

//------------------------------------------------------------------------------

std::string Recordset_sql_storage::statements_as_sql_script(const Sql_script::Statements &statements) {
  std::string sql_script;
  for (const auto &statement : statements)
    sql_script += statement + ";\n";
  return sql_script;
}

Recordset_sql_storage::Recordset_sql_storage()
  : Recordset_data_storage(),
    _is_sql_script_substitute_enabled(false),
    _omit_schema_qualifier(false),
    _binding_blobs(true) {
}

Recordset_sql_storage::~Recordset_sql_storage() {
}

void Recordset_sql_storage::init_variant_quoter(sqlide::QuoteVar &qv) const {
  if (_rdbms.is_valid()) {
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(_rdbms);
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
    qv.escape_string = sql_specifics->escape_sql_string();
    qv.store_unknown_as_string = false;
    qv.allow_func_escaping = true;
  } else {
    // used for sqlite storage and others
    qv.escape_string = std::bind(sqlide::QuoteVar::escape_ansi_sql_string, std::placeholders::_1);
    // swap db (sqlite) stores unknown values as quoted strings
    qv.store_unknown_as_string = true;
    qv.allow_func_escaping = false;
  }
  qv.blob_to_string = (_binding_blobs)
                        ? sqlide::QuoteVar::Blob_to_string()
                        : std::bind(sqlide::QuoteVar::blob_to_hex_string, std::placeholders::_1, std::placeholders::_2);
}

void Recordset_sql_storage::do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db) {
  Recordset::Column_names &column_names = get_column_names(recordset);
  Recordset::Column_types &column_types = get_column_types(recordset);
  Recordset::Column_types &real_column_types = get_real_column_types(recordset);
  Recordset::Column_flags &column_flags = get_column_flags(recordset);

  _pkey_columns.clear();
  _fields_order.clear(); //! make auto var & bind like var_list

  if (!sql_script().empty()) // load data from sql script
  {
    Var_list var_list;
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql"); //!
    Sql_inserts_loader::Ref loader = sql_facade->sqlInsertsLoader();
    loader->process_insert_cb(std::bind(&Recordset_sql_storage::load_insert_statement, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                                        std::placeholders::_5, &column_names, &var_list));
    loader->load(sql_script(), schema_name());

    // column types
    column_types.reserve(column_names.size());
    std::fill_n(std::back_inserter(column_types), column_names.size(), std::string());

    // real column types
    real_column_types.reserve(column_names.size());
    std::fill_n(std::back_inserter(real_column_types), column_names.size(), std::string());

    column_flags.reserve(column_names.size());
    std::fill_n(std::back_inserter(column_flags), column_names.size(), Recordset::NeedsQuoteFlag);

    if (!column_names.empty()) {
      // data
      {
        sqlide::Sqlite_transaction_guarder transaction_guarder(data_swap_db);

        create_data_swap_tables(data_swap_db, column_names, column_types);
        Var_vector row_values(column_names.size());
        std::list<std::shared_ptr<sqlite::command> > insert_commands =
          prepare_data_swap_record_add_statement(data_swap_db, column_names);
        Var_list::iterator var_list_iter = var_list.begin();
        for (size_t n = 0, count = var_list.size() / column_names.size(); n < count; ++n) {
          for (size_t l = 0, count = column_names.size(); l < count; ++l)
            row_values[l] = *var_list_iter++;
          add_data_swap_record(insert_commands, row_values);
        }

        transaction_guarder.commit();
      }
    }

    _readonly = column_names.empty();
    _valid = !column_names.empty();
  } else {
    // Is read only if it is a non editable select in which case
    // schema name and table name will be empty
    _readonly = !_sql_query.empty() && full_table_name().empty();
    _valid = false;
  }
}

void Recordset_sql_storage::load_insert_statement(const std::string &sql,
                                                  const std::pair<std::string, std::string> &schema_table,
                                                  const Sql_inserts_loader::Strings &fields_names,
                                                  const Sql_inserts_loader::Strings &fields_values,
                                                  const std::vector<bool> &null_fields,
                                                  Recordset::Column_names *column_names, Var_list *var_list) {
  if ((schema_table.first != _schema_name) || (schema_table.second != _table_name)) {
    grt::GRT::get()->send_error("Irrelevant insert statement (skipped): " + sql);
    return;
  }

  if (fields_names.size() != fields_values.size()) {
    grt::GRT::get()->send_error("Invalid insert statement: " + sql);
    return;
  }

  // 1st insert statement defines the set & order of fields in recordset
  // but only if affective_columns was not set
  if (_fields_order.empty()) {
    *column_names = _affective_columns.empty() ? fields_names : _affective_columns;
    for (const auto &fn : *column_names)
      _fields_order.insert(std::make_pair(fn, (int)_fields_order.size()));
  }

  // check fields names & determine fields order
  std::map<int, int> col_index_map; // index of field in var_list : index of field in passed fields_names
  for (ColumnId n = 0, count = fields_names.size(); n < count; ++n) {
    Fields_order::const_iterator i = _fields_order.find(fields_names[n]);
    if (_fields_order.end() != i)
      col_index_map[i->second] = (int)n;
  }

  // insert row
  for (ColumnId n = 0, count = _fields_order.size(); n < count; ++n) {
    std::map<int, int>::const_iterator i = col_index_map.find((int)n);
    if ((col_index_map.end() != i) && !null_fields[i->second])
      var_list->push_back(fields_values[i->second]);
    else
      var_list->push_back(sqlite::null_t());
  }
}

void Recordset_sql_storage::do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db) {
  _sql_script = std::string();
  Sql_script sql_script;
  generate_inserts(recordset, data_swap_db, sql_script);
  std::ostringstream oss;
  std::copy(sql_script.statements.begin(), sql_script.statements.end(), std::ostream_iterator<std::string>(oss, ";\n"));
  _sql_script = oss.str();
}

void Recordset_sql_storage::do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db,
                                             bool skip_commit) {
  if (_table_name.empty())
    return;

  Sql_script sql_script;
  generate_sql_script(recordset, data_swap_db, sql_script, true);
  run_sql_script(sql_script, skip_commit);
}

void Recordset_sql_storage::fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid,
                                             ColumnId column, sqlite::variant_t &blob_value) {
  blob_value = sqlite::null_t();

  // first check if requested blob is already in cache
  {
    size_t partition = Recordset::data_swap_db_column_partition(column);
    std::string partition_suffix = Recordset::data_swap_db_partition_suffix(partition);
    sqlite::query blob_query(*data_swap_db, base::strfmt("select `_%u` from `data%s` where `id`=?",
                                                         (unsigned int)column, partition_suffix.c_str()));
    blob_query % (int)rowid;
    if (blob_query.emit()) {
      std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(blob_query.get_result());
      blob_value = rs->get_variant(0);
    }
  }

  if (!recordset->optimized_blob_fetching() || !sqlide::is_var_null(blob_value))
    return;

  Recordset_data_storage::fetch_blob_value(recordset, data_swap_db, rowid, column, blob_value);
}

void Recordset_sql_storage::do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid,
                                                ColumnId column, sqlite::variant_t &blob_value) {
}

std::string Recordset_sql_storage::full_table_name() const {
  if (_table_name.empty())
    return "";

  std::string res = "`" + _table_name + "`";
  if (!_schema_name.empty())
    res = "`" + _schema_name + "`." + res;

  return res;
}

void Recordset_sql_storage::init_sql_script_substitute(const Recordset::Ptr &recordset_ptr, bool is_update_script) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, recordset_ptr, recordset)
  std::shared_ptr<sqlite::connection> data_swap_db = this->data_swap_db(recordset_ref);
  do_init_sql_script_substitute(recordset, data_swap_db.get(), is_update_script);
}

void Recordset_sql_storage::do_init_sql_script_substitute(const Recordset *recordset, sqlite::connection *data_swap_db,
                                                          bool is_update_script) {
  // temporarily disable is_sql_script_substitute_enabled flag to allow generation of sql script
  bool temporarily_disabled = false;
  AutoSwap<bool> flag_keeper(_is_sql_script_substitute_enabled, temporarily_disabled);

  _sql_script_substitute.reset();
  generate_sql_script(recordset, data_swap_db, _sql_script_substitute, is_update_script);
}

void Recordset_sql_storage::omit_schema_qualifier(bool flag) {
  _omit_schema_qualifier = flag;
}

void Recordset_sql_storage::get_pkey_predicate_for_data_cache_rowid(Recordset *recordset,
                                                                    sqlite::connection *data_swap_db, RowId rowid,
                                                                    std::string &pkey_predicate) {
  Recordset::Column_names &column_names = get_column_names(recordset);
  Recordset::Column_types &column_types = get_column_types(recordset);

  std::list<std::shared_ptr<sqlite::query> > data_row_queries(recordset->data_swap_db_partition_count());
  Recordset::prepare_partition_queries(data_swap_db, "select * from `data%s` where id=?", data_row_queries);

  std::vector<std::shared_ptr<sqlite::result> > data_row_results(data_row_queries.size());
  std::list<sqlite::variant_t> bind_vars;
  bind_vars.push_back((int)rowid);
  if (!Recordset::emit_partition_queries(data_swap_db, data_row_queries, data_row_results, bind_vars))
    return;

  sqlide::QuoteVar qv;
  init_variant_quoter(qv);
  // turn blob values into hex strings when building a primary key, since we can't bind those
  //  qv.blob_to_string= std::bind(sqlide::QuoteVar::blob_to_hex_string, std::placeholders::_1, std::placeholders::_2);
  PrimaryKeyPredicate pkey_pred(&column_types, &column_names, &_pkey_columns, &qv);
  pkey_predicate = pkey_pred(data_row_results);
}

void Recordset_sql_storage::generate_sql_script(const Recordset *recordset, sqlite::connection *data_swap_db,
                                                Sql_script &sql_script, bool is_update_script, bool binaryAsString) {
  // this one is used mostly internally, for eg.: sqlite inserts storage

  if (_is_sql_script_substitute_enabled) {
    sql_script = _sql_script_substitute;
    return;
  }

  const Recordset::Column_names &column_names = get_column_names(recordset);
  const Recordset::Column_types &column_types = get_column_types(recordset);
  const Recordset::Column_types &real_column_types = get_real_column_types(recordset);
  const Recordset::Column_flags &column_flags = get_column_flags(recordset);
  const Recordset::DBColumn_types &dbColumnTypes = getDbColumnTypes(recordset);
  const size_t partition_count = recordset->data_swap_db_partition_count();
  const sqlide::VarToStr var_to_str;

  RowId min_new_rowid = recordset->min_new_rowid();
  ColumnId editable_col_count = recordset->get_column_count();
  sqlide::QuoteVar qv;
  init_variant_quoter(qv);

  sqlide::QuoteVar pk_qv;
  init_variant_quoter(pk_qv);
  // turn blob values into hex strings when building a primary key, since we can't bind those
  pk_qv.blob_to_string = std::bind(sqlide::QuoteVar::blob_to_hex_string, std::placeholders::_1, std::placeholders::_2);

  std::string full_table_name = this->full_table_name();

  JsonTypeFinder jsonTypeFinder;
  bool unknownAsStringOrginal = qv.store_unknown_as_string;
  if (0 == editable_col_count)
    return;

  std::vector<bool> blob_columns(editable_col_count);
  for (ColumnId col = 0; editable_col_count > col; ++col)
    blob_columns[col] = sqlide::is_var_blob(real_column_types[col]);

  if (is_update_script) {
    PrimaryKeyPredicate pkey_pred(&real_column_types, &column_names, &_pkey_columns, &pk_qv);

    std::list<std::shared_ptr<sqlite::query> > data_row_queries(partition_count);
    Recordset::prepare_partition_queries(data_swap_db, "select * from `data%s` where id = ?", data_row_queries);

    std::list<std::shared_ptr<sqlite::query> > deleted_row_queries(partition_count);
    Recordset::prepare_partition_queries(data_swap_db, "select * from `deleted_rows%s` where id = ?",
                                         deleted_row_queries);

    sqlite::query changed_row_columns_query(*data_swap_db,
                                            "select\n"
                                            "c.`column`\n"
                                            "from `changes` c\n"
                                            "where `record` = ? and `action` = 0\n"
                                            "group by c.`column`\n"
                                            "order by 1");
    sqlite::query changes_query(*data_swap_db,
                                "select\n"
                                "min(c.id) as id,\n"
                                "c.`record`,\n"
                                "case\n"
                                "when exists(select 1 from `deleted_rows` where id=c.`record`) then -1\n"
                                "when `record` >= ? then 1\n"
                                "else 0\n"
                                "end as `action`\n"
                                "from `changes` c\n"
                                "group by c.`record`\n"
                                "having `record` < ? or not exists(select 1 from `deleted_rows` where id = c.`record`)\n"
                                "order by 1");

    changes_query % (int)min_new_rowid;
    changes_query % (int)min_new_rowid;
    if (changes_query.emit()) {
      std::shared_ptr<sqlite::result> rs = BoostHelper::convertPointer(changes_query.get_result());
      do {
        RowId rowid = rs->get_int(1);
        std::string sql;
        Sql_script::Statement_bindings sql_bindings;

        switch (rs->get_int(2)) // action
        {
          case -1: {
            std::vector<std::shared_ptr<sqlite::result> > deleted_row_results(deleted_row_queries.size());
            std::list<sqlite::variant_t> bind_vars;
            bind_vars.push_back((int)rowid);
            if (Recordset::emit_partition_queries(data_swap_db, deleted_row_queries, deleted_row_results, bind_vars)) {
              sql = strfmt("DELETE FROM %s WHERE %s", full_table_name.c_str(), pkey_pred(deleted_row_results).c_str());
            }
          } break;

          case 1: {
            std::vector<std::shared_ptr<sqlite::result> > data_row_results(data_row_queries.size());
            {
              std::list<sqlite::variant_t> bind_vars;
              bind_vars.push_back((int)rowid);
              if (!Recordset::emit_partition_queries(data_swap_db, data_row_queries, data_row_results, bind_vars))
                break;
            }

            changed_row_columns_query.clear();
            changed_row_columns_query % (int)rowid;
            if (!changed_row_columns_query.emit())
              break;

            {
              std::shared_ptr<sqlite::result> changed_row_columns_rs =
                BoostHelper::convertPointer(changed_row_columns_query.get_result());

              std::string col_names;
              std::string values;
              sqlite::variant_t v;

              do {
                ColumnId column = changed_row_columns_rs->get_int(0);
                col_names += strfmt("`%s`, ", column_names[column].c_str());

                size_t partition;
                ColumnId partition_column = Recordset::translate_data_swap_db_column(column, &partition);
                std::shared_ptr<sqlite::result> &data_row_rs = data_row_results[partition];

                v = data_row_rs->get_variant((int)partition_column);
                if (!qv.store_unknown_as_string && boost::apply_visitor(jsonTypeFinder, column_types[column], v))
                  qv.store_unknown_as_string = true;

                if (!binaryAsString && !dbColumnTypes.empty() && dbColumnTypes[column] == "BIT")
                  qv.bitMode = !dbColumnTypes.empty() && dbColumnTypes[column] == "BIT";

                values += strfmt("%s, ", boost::apply_visitor(qv, column_types[column], v).c_str());

                if (unknownAsStringOrginal != qv.store_unknown_as_string)
                  qv.store_unknown_as_string = unknownAsStringOrginal;
                if (blob_columns[column])
                  sql_bindings.push_back(v);
              } while (changed_row_columns_rs->next_row());
              if (!col_names.empty())
                col_names.resize(col_names.size() - 2);
              if (!values.empty())
                values.resize(values.size() - 2);
              sql = strfmt("INSERT INTO %s (%s) VALUES (%s)",
                           _omit_schema_qualifier ? (std::string("`") + table_name() + std::string("`")).c_str()
                                                  : full_table_name.c_str(),
                           col_names.c_str(), values.c_str());
            }
          } break;

          case 0: {
            std::vector<std::shared_ptr<sqlite::result> > data_row_results(data_row_queries.size());
            {
              std::list<sqlite::variant_t> bind_vars;
              bind_vars.push_back((int)rowid);
              if (!Recordset::emit_partition_queries(data_swap_db, data_row_queries, data_row_results, bind_vars))
                break;
            }

            changed_row_columns_query.clear();
            changed_row_columns_query % (int)rowid;
            if (changed_row_columns_query.emit()) {
              std::shared_ptr<sqlite::result> changed_row_columns_rs =
                BoostHelper::convertPointer(changed_row_columns_query.get_result());

              std::string values;
              sqlite::variant_t v;
              do {
                ColumnId column = changed_row_columns_rs->get_int(0);

                size_t partition;
                ColumnId partition_column = Recordset::translate_data_swap_db_column(column, &partition);
                std::shared_ptr<sqlite::result> &data_row_rs = data_row_results[partition];

                v = data_row_rs->get_variant((int)partition_column);

                if (!qv.store_unknown_as_string && boost::apply_visitor(jsonTypeFinder, column_types[column], v))
                  qv.store_unknown_as_string = true;
                values += strfmt("`%s` = %s, ", column_names[column].c_str(),
                                 boost::apply_visitor(qv, column_types[column], v).c_str());
                if (unknownAsStringOrginal != qv.store_unknown_as_string)
                  qv.store_unknown_as_string = unknownAsStringOrginal;
                if (blob_columns[column] && _binding_blobs)
                  sql_bindings.push_back(v);
              } while (changed_row_columns_rs->next_row());
              if (!values.empty())
                values.resize(values.size() - 2);
              sql = strfmt("UPDATE %s SET %s WHERE %s", full_table_name.c_str(), values.c_str(),
                           pkey_pred(data_row_results).c_str());
            }
          } break;
        }

        sql_script.statements.push_back(sql);
        sql_script.statements_bindings.push_back(sql_bindings);
      } while (rs->next_row());
    }
  } else {
    std::string col_names;
    for (ColumnId col = 0; editable_col_count > col; ++col)
      col_names += strfmt("`%s`, ", column_names[col].c_str());
    if (!col_names.empty())
      col_names.resize(col_names.size() - 2);

    std::list<std::shared_ptr<sqlite::query> > data_queries(partition_count);
    Recordset::prepare_partition_queries(data_swap_db, "select * from `data%s`", data_queries);
    std::vector<std::shared_ptr<sqlite::result> > data_results(data_queries.size());
    if (Recordset::emit_partition_queries(data_swap_db, data_queries, data_results)) {
      bool next_row_exists = true;
      do {
        sqlite::variant_t v;
        std::string values;
        for (size_t partition = 0; partition < partition_count; ++partition) {
          std::shared_ptr<sqlite::result> &data_rs = data_results[partition];
          for (ColumnId col_begin = partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT, col = col_begin,
                        col_end = std::min<ColumnId>(editable_col_count,
                                                     (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
               col < col_end; ++col) {
            ColumnId partition_column = col - col_begin;
            v = data_rs->get_variant((int)partition_column);
            values +=
              strfmt("%s, ", (column_flags[partition_column] & Recordset::NeedsQuoteFlag) || sqlide::is_var_null(v)
                               ? boost::apply_visitor(qv, column_types[partition_column], v).c_str()
                               : boost::apply_visitor(var_to_str, v).c_str());
          }
        }
        if (!values.empty())
          values.resize(values.size() - 2);
        std::string sql = strfmt("INSERT INTO %s (%s) VALUES (%s)",
                                 _omit_schema_qualifier ? (std::string("`") + table_name() + std::string("`")).c_str()
                                                        : full_table_name.c_str(),
                                 col_names.c_str(), values.c_str());
        sql_script.statements.push_back(sql);

        for (auto &data_rs : data_results)
          next_row_exists = data_rs->next_row();
      } while (next_row_exists);
    }
  }
}

void Recordset_sql_storage::generate_inserts(const Recordset *recordset, sqlite::connection *data_swap_db,
                                             Sql_script &sql_script) {
  // This one is used for generating inserts for export

  if (_is_sql_script_substitute_enabled) {
    sql_script = _sql_script_substitute;
    return;
  }

  const Recordset::Column_names &column_names = get_column_names(recordset);
  const Recordset::Column_types &column_types = get_column_types(recordset);
  const Recordset::Column_types &real_column_types = get_real_column_types(recordset);
  const Recordset::Column_flags &column_flags = get_column_flags(recordset);
  const Recordset::DBColumn_types &dbColumnTypes = getDbColumnTypes(recordset);
  const size_t partition_count = recordset->data_swap_db_partition_count();

  // RowId min_new_rowid= recordset->min_new_rowid();
  ColumnId editable_col_count = recordset->get_column_count();
  sqlide::QuoteVar qv;
  init_variant_quoter(qv);
  qv.store_unknown_as_string = true;
  std::string full_table_name = this->full_table_name();

  if (0 == editable_col_count)
    return;

  std::vector<bool> blob_columns(editable_col_count);
  for (ColumnId col = 0; editable_col_count > col; ++col)
    blob_columns[col] = sqlide::is_var_blob(real_column_types[col]);

  std::string col_names;
  for (ColumnId col = 0; editable_col_count > col; ++col)
    col_names += strfmt("`%s`, ", column_names[col].c_str());
  if (!col_names.empty())
    col_names.resize(col_names.size() - 2);

  std::list<std::shared_ptr<sqlite::query> > data_queries(partition_count);
  Recordset::prepare_partition_queries(data_swap_db, "select * from `data%s`", data_queries);
  std::vector<std::shared_ptr<sqlite::result> > data_results(data_queries.size());
  if (Recordset::emit_partition_queries(data_swap_db, data_queries, data_results)) {
    bool next_row_exists = true;
    do {
      sqlite::variant_t v;
      std::string values;
      for (size_t partition = 0; partition < partition_count; ++partition) {
        std::shared_ptr<sqlite::result> &data_rs = data_results[partition];
        for (ColumnId col_begin = partition * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT, col = col_begin,
                      col_end = std::min<ColumnId>(editable_col_count,
                                                   (partition + 1) * Recordset::DATA_SWAP_DB_TABLE_MAX_COL_COUNT);
             col < col_end; ++col) {
          ColumnId partition_column = col - col_begin;
          v = data_rs->get_variant((int)partition_column);

          std::string value;
          if (sqlide::is_var_null(v) && (column_flags[col] & Recordset::NotNullFlag) != 0) {
              value = "DEFAULT";
          } else {

            qv.bitMode = !dbColumnTypes.empty() && dbColumnTypes[col] == "BIT";
            qv.needQuote = column_flags[col] & Recordset::NeedsQuoteFlag;

            value = strfmt("%s", boost::apply_visitor(qv, column_types[col], v).c_str());
          }

          values += strfmt("%s, ", value.c_str());
        }
      }
      if (!values.empty())
        values.resize(values.size() - 2);
      std::string sql = strfmt(
        "INSERT INTO %s (%s) VALUES (%s)",
        _omit_schema_qualifier ? (std::string("`") + table_name() + std::string("`")).c_str() : full_table_name.c_str(),
        col_names.c_str(), values.c_str());
      sql_script.statements.push_back(sql);

      for (auto &data_rs : data_results)
        next_row_exists = data_rs->next_row();
    } while (next_row_exists);
  }
}
