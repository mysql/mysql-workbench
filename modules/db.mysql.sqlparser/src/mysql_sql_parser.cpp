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

#if defined(_MSC_VER)
#include <windows.h>
#endif

#include <glib.h>
#include <boost/signals2.hpp>
#include <cctype>

#include "base/util_functions.h"

#include "mysql_sql_parser.h"
#include "grtsqlparser/module_utils.h"
#include "mysql_sql_parser_utils.h"
#include "grtdb/db_object_helpers.h"

#include "base/string_utilities.h"
#include <sstream>
#include <iterator>
#include <boost/lambda/bind.hpp>

using namespace grt;
using namespace base;

class TableStorageEngines {
public:
  TableStorageEngines() {
  }
  void init() {
    grt::ListRef<db_mysql_StorageEngine> engines;
    grt::Module *module = grt::GRT::get()->get_module("DbMySQL");
    if (!module)
      throw std::logic_error("module DbMySQL not found");
    grt::BaseListRef args(true);
    engines = grt::ListRef<db_mysql_StorageEngine>::cast_from(module->call_function("getKnownEngines", args));
    if (!engines.is_valid())
      throw std::logic_error("no known storage engines");
    for (grt::ListRef<db_mysql_StorageEngine>::const_iterator iter = engines.begin(); iter != engines.end(); ++iter) {
      const std::string name = (*iter)->name();
      _names_index[base::tolower(name)] = name;
    }
  }

  std::string normalize_name(const std::string &name) const {
    std::string lower_cased_name = tolower(name);
    std::map<std::string, std::string>::const_iterator i = _names_index.find(lower_cased_name);
    return (_names_index.end() == i) ? name : i->second;
  }

private:
  std::map<std::string, std::string> _names_index;
};
static TableStorageEngines __table_storage_engines;

Mysql_sql_parser::Null_state_keeper::~Null_state_keeper() {
  _sql_parser->_fk_refs.clear();
  boost::function<Parse_result()> f = boost::lambda::constant(pr_irrelevant);
  _sql_parser->_process_specific_create_statement.clear();
  _sql_parser->_datatype_cache = grt::DictRef();
  _sql_parser->_created_objects = grt::ListRef<GrtObject>();
  _sql_parser->_processing_create_statements = true;
  _sql_parser->_processing_alter_statements = true;
  _sql_parser->_processing_drop_statements = true;
  _sql_parser->_set_old_names = true;
  _sql_parser->_reuse_existing_objects = false;
  _sql_parser->_reusing_existing_obj = false;
  _sql_parser->_stick_to_active_schema = false;
  _sql_parser->_gen_fk_names_when_empty = true;
  _sql_parser->_strip_sql = true;
  _sql_parser->_last_parse_result = pr_irrelevant;
  _sql_parser->_sql_script_codeset = StringRef("");
  _sql_parser->_triggers_owner_table = db_mysql_TableRef();

  _sql_parser->_shape_schema = boost::bind(f);
  _sql_parser->_shape_table = boost::bind(f);
  _sql_parser->_shape_view = boost::bind(f);
  _sql_parser->_shape_routine = boost::bind(f);
  _sql_parser->_shape_trigger = boost::bind(f);
  _sql_parser->_shape_index = boost::bind(f);
  _sql_parser->_shape_logfile_group = boost::bind(f);
  _sql_parser->_shape_tablespace = boost::bind(f);
  _sql_parser->_shape_serverlink = boost::bind(f);

  static struct TableStorageEnginesInitializer {
    TableStorageEnginesInitializer() {
      __table_storage_engines.init();
    }
  } table_storage_engines_initializer;
}
#define NULL_STATE_KEEPER Null_state_keeper _nsk(this);

#define ACTIVE_SCHEMA_KEEPER Active_schema_keeper _sk(this);

#define ENSURE(call, text)             \
  try {                                \
    call;                              \
  } catch (const Parse_exception &e) { \
    std::string err_text = text;       \
    throw Parse_exception(err_text);   \
  }

Mysql_sql_parser::Mysql_sql_parser()
  : _processing_create_statements(false),
    _processing_alter_statements(false),
    _processing_drop_statements(false),
    _set_old_names(false),
    _reuse_existing_objects(false),
    _reusing_existing_obj(false),
    _stick_to_active_schema(false),
    _gen_fk_names_when_empty(false),
    _strip_sql(false),
    _last_parse_result(Sql_parser_base::pr_irrelevant)

{
  NULL_STATE_KEEPER
}

void Mysql_sql_parser::set_options(const grt::DictRef &options) {
  Mysql_sql_parser_base::set_options(options);

  if (!options.is_valid())
    return;

  overwrite_default_option<grt::StringRef>(_sql_script_codeset, "sql_script_codeset", options, true);
  overwrite_default_option(_created_objects, "created_objects", options, false);
  overwrite_default_option<grt::IntegerRef>(_gen_fk_names_when_empty, "gen_fk_names_when_empty", options);
  overwrite_default_option<grt::IntegerRef>(_case_sensitive_identifiers, "case_sensitive_identifiers", options);
  overwrite_default_option<grt::IntegerRef>(_processing_create_statements, "processing_create_statements", options);
  overwrite_default_option<grt::IntegerRef>(_processing_alter_statements, "processing_alter_statements", options);
  overwrite_default_option<grt::IntegerRef>(_processing_drop_statements, "processing_drop_statements", options);
  overwrite_default_option<grt::IntegerRef>(_reuse_existing_objects, "reuse_existing_objects", options);
}

int Mysql_sql_parser::parse_sql_script(db_CatalogRef catalog, const std::string &sql, grt::DictRef options) {
  return parse_sql_script(catalog, sql, false, options);
}

int Mysql_sql_parser::parse_sql_script_file(db_CatalogRef catalog, const std::string &filename, grt::DictRef options) {
  return parse_sql_script(catalog, filename, true, options);
}

int Mysql_sql_parser::parse_sql_script(db_CatalogRef &catalog, const std::string &sql, bool from_file,
                                       grt::DictRef &options) {
  if (!catalog.is_valid())
    return pr_invalid;

  NULL_STATE_KEEPER

  _catalog = db_mysql_CatalogRef::cast_from(catalog);

  set_options(options);

  add_log_message("Started parsing MySQL SQL script.", 3);

  set_progress_state(0.f, _("Parsing MySQL SQL Script..."));

  build_datatype_cache();

  // change current schema to default, it will be used for objects without specified schema
  db_mysql_SchemaRef default_schema;
  int initial_schemata_count = -1;

  initial_schemata_count = (int)_catalog->schemata().count();
  if (0 == initial_schemata_count)
    default_schema = set_active_schema("default_schema"); // this will add schema to schemata if necessary
  else {
    default_schema = db_mysql_SchemaRef::cast_from(_catalog->defaultSchema());
    if (!default_schema.is_valid())
      default_schema = _catalog->schemata().get(0);
    set_active_schema(*default_schema->name());
  }

  _process_sql_statement = boost::bind(&Mysql_sql_parser::process_sql_statement, this, _1);

  int res;
  Mysql_sql_parser_fe sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"));
  sql_parser_fe.processing_create_statements = _processing_create_statements;
  sql_parser_fe.processing_alter_statements = _processing_alter_statements;
  sql_parser_fe.processing_drop_statements = _processing_drop_statements;

  const std::string *sql_script = &sql;
  std::string sql_in_utf8;
  if (!_sql_script_codeset.empty() && (_sql_script_codeset != "UTF8")) {
    std::ifstream ifs(sql.c_str(), std::ios_base::in | std::ios_base::binary);
    if (ifs) {
      ifs >> std::noskipws;
      std::string sql_original;
      std::copy(std::istream_iterator<char>(ifs), std::istream_iterator<char>(), std::back_inserter(sql_original));

      gsize bytes_read;
      gsize bytes_written;
      GError *error = NULL;
      char *sql_converted = g_convert(sql_original.c_str(), sql_original.length(), "UTF-8", _sql_script_codeset.c_str(),
                                      &bytes_read, &bytes_written, &error);
      if (!error) {
        sql_in_utf8 = sql_converted;
        from_file = false;
        sql_script = &sql_in_utf8;
      } else
        g_free(error);
      g_free(sql_converted);
    }
  }

  if (from_file)
    res = Mysql_sql_parser_base::parse_sql_script_file(sql_parser_fe, *sql_script);
  else
    // TODO: optimize to avoid copies of the input (especially for large scripts).
    res = Mysql_sql_parser_base::parse_sql_script(sql_parser_fe, sql_script->c_str());

  set_progress_state(0.9f, _("Creating foreign key references..."));

  // 2-nd stage: now, when all tables are created it's safe to set foreign key references
  set_fk_references();

  // remove default schema if it was created by this procedure & after all it's empty
  if (0 == initial_schemata_count && default_schema.is_valid() && 0 == default_schema->tables().count() &&
      0 == default_schema->views().count() && 0 == default_schema->routines().count())
    _catalog->schemata().remove_value(default_schema);

  set_progress_state(1.f, _("Finished parsing MySQL SQL script."));

  {
    std::ostringstream oss;
    oss << "Finished parsing MySQL SQL script. Totally processed statements: successful (" << _processed_obj_count
        << "), errors (" << _err_count << "), warnings (" << _warn_count << ").";
    add_log_message(oss.str(), 3);
  }

  return res;
}

int Mysql_sql_parser::process_sql_statement(const SqlAstNode *tree) {
  _reusing_existing_obj = false;
  _last_parse_result = pr_irrelevant;

  if (!tree) {
    report_sql_error(_err_tok_lineno, true, _err_tok_line_pos, _err_tok_len, _err_msg, 2);
    _last_parse_result = pr_invalid;
    return 1;
  }

  try {
    _last_parse_result = pr_irrelevant;

    if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_create))
      _last_parse_result = process_create_statement(item);
    else if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_drop))
      _last_parse_result = process_drop_statement(item);
    else if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_alter))
      _last_parse_result = process_alter_statement(item);
    else if (const SqlAstNode *item = tree->subitem(sql::_statement, sql::_use))
      process_use_schema_statement(item);

    if (pr_processed == _last_parse_result)
      ++_processed_obj_count; // count only processed statements (for clarity)
  } catch (const Parse_exception &e) {
    std::string msg_text =
      e.what() + EOL + "SQL statement" + EOL + cut_sql_statement(strip_sql_statement(sql_statement(), true));
    add_log_message(msg_text, e.flag());
    ++_err_count;

    _last_parse_result = pr_invalid;
    return 1; // error count
  }

  return 0; // error count
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_statement(const SqlAstNode *tree) {
  typedef Parse_result (Mysql_sql_parser::*statement_processor)(const SqlAstNode *);
  static statement_processor proc_arr[] = {
    &Mysql_sql_parser::process_create_table_statement,      &Mysql_sql_parser::process_create_index_statement,
    &Mysql_sql_parser::process_create_view_statement,       &Mysql_sql_parser::process_create_routine_statement,
    &Mysql_sql_parser::process_create_trigger_statement,    &Mysql_sql_parser::process_create_server_link_statement,
    &Mysql_sql_parser::process_create_tablespace_statement, &Mysql_sql_parser::process_create_logfile_group_statement,
    &Mysql_sql_parser::process_create_schema_statement,
  };

  if (_process_specific_create_statement)
    return _process_specific_create_statement(tree);
  else {
    for (size_t n = 0; n < ARR_CAPACITY(proc_arr); ++n) {
      statement_processor proc = proc_arr[n];
      Parse_result result = (this->*proc)(tree);
      if (pr_irrelevant != result)
        return result;
    }
  }

  return pr_irrelevant;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_drop_statement(const SqlAstNode *tree) {
  typedef Parse_result (Mysql_sql_parser::*statement_processor)(const SqlAstNode *);
  static statement_processor proc_arr[] = {
    &Mysql_sql_parser::process_drop_schema_statement,  &Mysql_sql_parser::process_drop_table_statement,
    &Mysql_sql_parser::process_drop_view_statement,    &Mysql_sql_parser::process_drop_routine_statement,
    &Mysql_sql_parser::process_drop_trigger_statement,
  };

  for (size_t n = 0; n < ARR_CAPACITY(proc_arr); ++n) {
    statement_processor proc = proc_arr[n];
    Parse_result result = (this->*proc)(tree);
    if (pr_irrelevant != result)
      return result;
  }

  return pr_irrelevant;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_alter_statement(const SqlAstNode *tree) {
  typedef Parse_result (Mysql_sql_parser::*statement_processor)(const SqlAstNode *);
  static statement_processor proc_arr[] = {
    &Mysql_sql_parser::process_alter_table_statement,
  };

  for (size_t n = 0; n < ARR_CAPACITY(proc_arr); ++n) {
    statement_processor proc = proc_arr[n];
    Parse_result result = (this->*proc)(tree);
    if (pr_irrelevant != result)
      return result;
  }

  return pr_irrelevant;
}

void Mysql_sql_parser::build_datatype_cache() {
  _datatype_cache = DictRef(true);
  ListRef<db_SimpleDatatype> datatypes = _catalog->simpleDatatypes();
  db_SimpleDatatypeRef datatype;
  for (size_t n = 0; n < datatypes.count(); n++) {
    datatype = datatypes.get(n);
    _datatype_cache.set(datatype->name(), datatype);
  }
}

void Mysql_sql_parser::do_transactable_list_insert(ListRef<GrtObject> list, GrtObjectRef object) {
  // this insert is important to be before check of _reusing_existing_obj
  // other classes rely on this order
  if (_created_objects.is_valid())
    _created_objects.insert(object);

  // time for this check is important - don't move
  if (_reusing_existing_obj)
    return;

  list.insert(object);
}

void Mysql_sql_parser::log_db_obj_created(const GrtNamedObjectRef &obj1, const GrtNamedObjectRef &obj2,
                                          const GrtNamedObjectRef &obj3) {
  if (_reusing_existing_obj)
    return;
  log_db_obj_operation("Created", obj1, obj2, obj3);
}

void Mysql_sql_parser::log_db_obj_dropped(const GrtNamedObjectRef &obj1, const GrtNamedObjectRef &obj2,
                                          const GrtNamedObjectRef &obj3) {
  log_db_obj_operation("Dropped", obj1, obj2, obj3);
}

void Mysql_sql_parser::log_db_obj_operation(const std::string &op_name, const GrtNamedObjectRef &obj1,
                                            const GrtNamedObjectRef &obj2, const GrtNamedObjectRef &obj3) {
  const GrtNamedObjectRef obj = obj3.is_valid() ? obj3 : (obj2.is_valid() ? obj2 : obj1);

  std::string text;
  text.append(op_name).append(" ").append(obj->get_metaclass()->get_attribute("caption")).append(": ");
  if (obj1.is_valid())
    text.append(obj1->name());
  if (obj2.is_valid())
    text.append(".").append(obj2->name());
  if (obj3.is_valid())
    text.append(".").append(obj3->name());
  add_log_message(text, 3);
}

void Mysql_sql_parser::set_fk_references() {
  grt::ListRef<db_mysql_Schema> schemata = _catalog->schemata();

  for (Fk_ref_collection::iterator i = _fk_refs.begin(); i != _fk_refs.end(); ++i) {
    // referred table
    db_mysql_SchemaRef ref_schema =
      find_named_object_in_list(schemata, i->ref_schema_name(), _case_sensitive_identifiers);
    db_mysql_TableRef ref_table;
    if (ref_schema.is_valid()) {
      ref_table = find_named_object_in_list(ref_schema->tables(), i->ref_table_name(), _case_sensitive_identifiers);
      if (!ref_table.is_valid()) {
        {
          std::string msg_text;
          msg_text.append("Table `")
            .append(i->ref_schema_name())
            .append("`.`")
            .append(i->ref_table_name())
            .append("` not found. Stub was created.");
          add_log_message(msg_text, 1);
        }
        create_stub_table(ref_schema, ref_table, i->ref_table_name());
      }
      (*i).operator db_ForeignKeyRef &()->referencedTable(ref_table);
    } else
      add_log_message(
        std::string("Could not find refschema ") + i->ref_schema_name() + " for reftable " + i->ref_table_name(), 1);

    // ref columns
    int column_index = 0;
    for (Fk_ref::String_collection::iterator c = i->ref_column_names().begin(); c != i->ref_column_names().end();
         ++c, ++column_index) {
      db_ForeignKeyRef &fk = *i;
      db_mysql_TableRef owner_table = db_mysql_TableRef::cast_from(i->owner_table());
      db_mysql_ColumnRef column =
        find_named_object_in_list(ref_table->columns(), *c, false); // mysql columns are always case-insensitive

      if (!column.is_valid()) {
        std::string msg_text;
        msg_text.append("Table `")
          .append(db_SchemaRef::cast_from(owner_table->owner())->name())
          .append("`.`")
          .append(owner_table->name())
          .append("` : Foreign key `")
          .append(fk->name())
          .append("` : Referred column `")
          .append(*ref_schema->name())
          .append("`.`")
          .append(*ref_table->name())
          .append("`.`")
          .append(*c)
          .append("` not found. ");

        if (ref_table->isStub()) {
          create_stub_column(ref_table, column, *c, db_mysql_ColumnRef::cast_from(fk->columns().get(column_index)));
          fk->referencedColumns().insert(column);
          if (ref_table->tableEngine().empty())
            ref_table->tableEngine(owner_table->tableEngine());
          msg_text.append("Stub was created.");
        } else {
          owner_table->foreignKeys().gremove_value(fk);
          msg_text.append("Foreign key was skipped.");
          //! throw Parse_exception(err_text);
        }

        add_log_message(msg_text, 1);
      } else {
        fk->referencedColumns().insert(column);

        // add index for foreign key if it doesn't exist yet
        ListRef<db_Column> fk_columns = fk->columns();
        size_t ref_col_count = fk_columns.count();
        db_IndexRef found_index;
        ListRef<db_Index> indices = owner_table->indices();
        {
          for (size_t n = 0, count = indices.count(); n < count; ++n) {
            db_IndexRef index = indices.get(n);
            ListRef<db_IndexColumn> ind_columns = index->columns();
            bool fk_index_exists = true;
            for (size_t n = 0, count = ind_columns.count(); n < ref_col_count; ++n) {
              if ((n >= count) || (fk_columns.get(n) != ind_columns.get(n)->referencedColumn())) {
                fk_index_exists = false;
                break;
              }
            }
            if (fk_index_exists) {
              found_index = index;
              break;
            }
          }
        }

        if (found_index.is_valid()) {
          if ((*found_index->indexType()).empty())
            found_index->indexType("INDEX");
          // found_index->indexType("FOREIGN");
          // new field for direct mapping --alfredo 10/04/16
          fk->index(found_index);
        } else {
          db_mysql_IndexRef index(grt::Initialized);
          index->owner(owner_table);
          set_obj_name(index, fk->name());
          // index->indexType("FOREIGN");
          index->indexType("INDEX");
          // new field for direct mapping --alfredo 10/04/16
          fk->index(index);

          for (size_t n = 0; n < ref_col_count; ++n) {
            db_ColumnRef column = fk_columns.get(n);
            db_mysql_IndexColumnRef index_column(grt::Initialized);
            index_column->owner(index);
            index_column->referencedColumn(column);
            index->columns().insert(index_column);
          }

          indices.insert(index);
        }
      }
    }
  }
}

void Mysql_sql_parser::set_obj_sql_def(db_DatabaseDdlObjectRef obj) {
  obj->sqlDefinition(strip_sql_statement(sql_statement(), _strip_sql));
}

void Mysql_sql_parser::set_obj_name(GrtNamedObjectRef obj, const std::string &val) {
  SET_STR(obj->name, val)
  if (_set_old_names)
    obj->oldName(obj->name());
}

db_mysql_SchemaRef Mysql_sql_parser::set_active_schema(const std::string &schema_name) {
  return _active_schema = ensure_schema_created(schema_name, false);
}

db_mysql_SchemaRef Mysql_sql_parser::ensure_schema_created(const std::string &schema_name,
                                                           bool check_obj_name_uniqueness) {
  if (schema_name.empty())
    return _active_schema;

  // try to find existing one first
  db_mysql_SchemaRef schema = find_named_object_in_list(_catalog->schemata(), schema_name, _case_sensitive_identifiers);
  if (!schema.is_valid()) // create if not found
  {
    schema = db_mysql_SchemaRef(grt::Initialized);
    schema->owner(_catalog);

    std::string time = base::fmttime(0, DATETIME_FMT);
    schema->createDate(time);
    schema->lastChangeDate(time);

    set_obj_name(schema, schema_name);

    {
      Cs_collation_setter schema_cs_collation_setter =
        cs_collation_setter(db_SchemaRef(schema), db_CatalogRef(_catalog), true);
      schema_cs_collation_setter.charset_name(_catalog->defaultCharacterSetName());
      schema_cs_collation_setter.collation_name(_catalog->defaultCollationName());
    }
    if (_shape_schema)
      _shape_schema(schema);
    do_transactable_list_insert(_catalog->schemata(), schema);
    log_db_obj_created(schema);
  } else if (check_obj_name_uniqueness)
    blame_existing_obj(false, schema);

  return schema;
}

void Mysql_sql_parser::create_stub_table(db_mysql_SchemaRef &schema, db_mysql_TableRef &obj,
                                         const std::string &obj_name) {
  obj = db_mysql_TableRef(grt::Initialized);
  obj->owner(schema);
  obj->isStub(1);
  set_obj_name(obj, obj_name);
  schema->tables().insert(obj);
}

void Mysql_sql_parser::create_stub_column(db_mysql_TableRef &table, db_mysql_ColumnRef &obj,
                                          const std::string &obj_name, db_mysql_ColumnRef tpl_obj) {
  obj = db_mysql_ColumnRef(grt::Initialized);
  obj->owner(table);
  set_obj_name(obj, obj_name);

  obj->simpleType(tpl_obj->simpleType());
  obj->userType(tpl_obj->userType());
  obj->structuredType(tpl_obj->structuredType());
  obj->precision(tpl_obj->precision());
  obj->scale(tpl_obj->scale());
  obj->length(tpl_obj->length());
  obj->datatypeExplicitParams(tpl_obj->datatypeExplicitParams());
  obj->formattedType(tpl_obj->formattedType());

  StringListRef tpl_flags = tpl_obj->flags();
  StringListRef flags = obj->flags();
  for (size_t c = tpl_flags.count(), i = 0; i < c; i++)
    flags.ginsert(tpl_flags.get(i));

  obj->characterSetName(tpl_obj->characterSetName());
  obj->collationName(tpl_obj->collationName());

  table->columns().insert(obj);
}

void Mysql_sql_parser::process_field_type_item(const SqlAstNode *item, db_mysql_ColumnRef &column) {
  if (item) {
    // datatype
    {
      db_SimpleDatatypeRef datatype = map_datatype(item, _datatype_cache);
      if (!datatype.is_valid()) {
        std::string sql_text = item->restore_sql_text(_sql_statement);
        std::string msg_text = "Mapping failed for datatype `" + sql_text + "`";
        add_log_message(msg_text, 1);
      } else
        column->simpleType(datatype);
    }

    // datatypeExplicitParams
    {
      const SqlAstNode *string_list_item = item->subitem(sql::_string_list);
      if (string_list_item) {
        std::string sql_text;
        sql_text.append("(").append(string_list_item->restore_sql_text(_sql_statement)).append(")");
        column->datatypeExplicitParams(sql_text);
      }
    }

    // length
    {
      static sql::symbol path1[] = {sql::_field_length, sql::_};
      static sql::symbol path2[] = {sql::_opt_field_length, sql::_field_length, sql::_};
      static sql::symbol *paths[] = {path1, path2};

      static sql::symbol names[] = {sql::_LONG_NUM, sql::_ULONGLONG_NUM, sql::_DECIMAL_NUM, sql::_NUM};

      const SqlAstNode *searched_item = item->search_by_paths(paths, ARR_CAPACITY(paths));
      if (searched_item)
        searched_item = searched_item->search_by_names(names, ARR_CAPACITY(names));
      if (column->simpleType().is_valid() && *column->simpleType()->numericPrecision() != bec::EMPTY_TYPE_PRECISION)
        SET_INT_I(column->precision, searched_item)
      else
        SET_INT_I(column->length, searched_item)
    }

    // float_options
    {
      std::string scale = "";
      std::string precision = "";
      const SqlAstNode *float_item = item->subitem(sql::_float_options);
      if (float_item)
        process_float_options_item(float_item, &precision, &scale);
      const SqlAstNode *precision_item = item->subitem(sql::_opt_precision);
      if (precision_item == NULL) {
        static sql::symbol precision_path[] = {sql::_real_type, sql::_precision, sql::_};
        precision_item = item->subitem_by_path(precision_path);
      }
      if (precision_item)
        process_float_options_item(precision_item, &precision, &scale);
      if (!scale.empty())
        SET_INT(column->scale, scale)
      if (!precision.empty())
        SET_INT(column->precision, precision)
    }

    // field options
    {
      StringListRef flags(column->flags());
      concatenate_items(item->subitem(sql::_field_options, sql::_field_opt_list), flags, true);
    }

    // charset
    {
      static sql::symbol path11[] = {sql::_opt_binary, sql::_ascii, sql::_};
      static sql::symbol path12[] = {sql::_opt_binary, sql::_unicode, sql::_};
      static sql::symbol path13[] = {sql::_opt_binary, sql::_};
      static sql::symbol *paths1[] = {path11, path12, path13};

      static sql::symbol path21[] = {sql::_charset_name, sql::_};
      static sql::symbol path22[] = {sql::_ASCII_SYM, sql::_};
      static sql::symbol path23[] = {sql::_BYTE_SYM, sql::_};
      static sql::symbol path24[] = {sql::_UNICODE_SYM, sql::_};
      static sql::symbol *paths2[] = {path21, path22, path23, path24};

      const SqlAstNode *searched_item = item->search_by_paths(paths1, ARR_CAPACITY(paths1));
      if (searched_item)
        searched_item = searched_item->search_by_paths(paths2, ARR_CAPACITY(paths2));
      if (searched_item)
        SET_STR_I(cs_collation_setter(column, db_mysql_TableRef::cast_from(column->owner()), false).charset_name,
                  searched_item)
    }

    // binary
    {
      static sql::symbol path1[] = {sql::_opt_binary, sql::_BINARY, sql::_};
      static sql::symbol path2[] = {sql::_opt_binary, sql::_opt_bin_mod, sql::_BINARY, sql::_};
      static sql::symbol path3[] = {sql::_opt_binary, sql::_charset_name, sql::_BINARY, sql::_};
      static sql::symbol path4[] = {sql::_opt_bin_mod, sql::_BINARY, sql::_};
      static sql::symbol *paths[] = {path1, path2, path3, path4};

      const SqlAstNode *searched_item = item->search_by_paths(paths, ARR_CAPACITY(paths));
      if (searched_item)
        column->flags().insert("BINARY");
    }
  }
}

void Mysql_sql_parser::process_field_attributes_item(const SqlAstNode *item, db_mysql_ColumnRef &column,
                                                     db_mysql_TableRef &table) {
  bool explicitDefaultValue = false;
  bool explicitNullValue = false;

  if (item) {
    // it was decided that defaultValue will also contain 'ON UPDATE CURRENT_TIMESTAMP' clause
    // do not remove this line until change is consistent with both subseq(sql::_DEFAULT) and subseq(sql::_ON,
    // UPDATE_SYM)
    for (SqlAstNode::SubItemList::const_iterator it = item->subitems()->begin(); it != item->subitems()->end(); ++it) {
      const SqlAstNode *subitem = *it;
      if (subitem->name_equals(sql::_attribute)) {
        const SqlAstNode *aux_item;

        if (subitem->subitem(sql::_AUTO_INC)) {
          // set auto_increment only for numeric types
          if (column->simpleType().is_valid() && column->simpleType()->group().is_valid() &&
              are_strings_eq_ci(column->simpleType()->group()->name(), "NUMERIC"))
            column->autoIncrement(1);
        } else if (subitem->subseq(sql::_not, sql::_NULL_SYM)) {
          column->isNotNull(1);
          explicitNullValue = true;
        } else if (subitem->subitem(sql::_NULL_SYM)) {
          column->isNotNull(0);
          explicitNullValue = true;
        } else if ((aux_item = subitem->subseq(sql::_COMMENT_SYM, sql::_TEXT_STRING_sys)))
          SET_STR_I(column->comment, aux_item)
        else if ((aux_item = subitem->subseq(sql::_COLLATE_SYM, sql::_collation_name)))
          SET_STR_I(cs_collation_setter(column, table, false).collation_name, aux_item)
        else if ((aux_item = subitem->subseq(sql::_DEFAULT, sql::_now_or_signed_literal))) {
          // this is a fix for 'ON UPDATE CURRENT_TIMESTAMP' case.
          // If we have have already assigned the text 'ON UPDATE CURRENT_TIMESTAMP'
          // to the default value, then now we should not overwrite but
          std::string text;
          if (column->defaultValue().is_valid())
            text.assign(column->defaultValue());

          SET_SQL_I(column->defaultValue, aux_item);

          if (!text.empty()) {
            std::string defv(column->defaultValue());
            defv.append(text);
            bec::ColumnHelper::set_default_value(column, defv);
          }

          if ((aux_item->subitem(sql::_signed_literal, sql::_literal, sql::_NULL_SYM)))
            column->defaultValueIsNull(1);
          explicitDefaultValue = true;
        }
        // This also handles the 'ON UPDATE CURRENT_TIMESTAMP' case as
        // CURRENT_TIMESTAMP is a synonymof NOW.
        else if ((aux_item = subitem->subseq(sql::_ON, sql::_UPDATE_SYM, sql::_now))) {
          std::string text;
          // it was decided that 'ON UPDATE CURRENT_TIMESTAMP' should be stored in defaultValue to not create new
          // rarely-used field
          if (column->defaultValue().is_valid()) {
            text = *column->defaultValue();
            text.append(" ");
          }

          // NOW is a subtree now with optional precision.
          aux_item = subitem->subitem(sql::_now);
          text.append(subitem->subitem(sql::_ON)->value())
            .append(" ")
            .append(subitem->subitem(sql::_UPDATE_SYM)->value())
            .append(" ")
            .append(aux_item->subitem(sql::_NOW_SYM)->value());

          bec::ColumnHelper::set_default_value(column, text);
        } else if ((aux_item = subitem->subitem(sql::_UNIQUE_SYM)) || (aux_item = subitem->subitem(sql::_KEY_SYM))) {
          db_mysql_IndexRef index(grt::Initialized);
          index->owner(table);

          // index type
          if (aux_item->name_equals(sql::_UNIQUE_SYM)) {
            index->unique(1);
            index->indexType("UNIQUE");
          } else {
            index->isPrimary(1);
            table->primaryKey(index);
            index->indexType("PRIMARY");
            // if ((*index->name()).empty()) // server resets any explicitly specified pk constraint names
            set_obj_name(index, "PRIMARY");
          }

          // index columns
          {
            db_mysql_IndexColumnRef index_column(grt::Initialized);
            index_column->owner(index);

            // column
            index_column->referencedColumn(column);

            index->columns().insert(index_column);
          }

          table->indices().insert(index);
        }
      }
    }
  }

  if (column->simpleType().is_valid() && are_strings_eq_ci(column->simpleType()->name(), "TIMESTAMP")) {
    if (!explicitNullValue)
      column->isNotNull(1);
  }

  if (!column->isNotNull() && !explicitDefaultValue)
    bec::ColumnHelper::set_default_value(column, "NULL");
}

std::string Mysql_sql_parser::process_field_name_item(const SqlAstNode *item, GrtNamedObjectRef obj, std::string *name3,
                                                      std::string *name2, std::string *name1) {
  std::string name = "";

  if (name1)
    name1->clear();
  if (name2)
    name2->clear();
  if (name3)
    name3->clear();

  if (item) {
    size_t n = 4;
    for (SqlAstNode::SubItemList::const_reverse_iterator it = item->subitems()->rbegin(),
                                                         end = item->subitems()->rend();
         it != end; ++it) {
      const SqlAstNode *subitem = *it;
      if (!subitem->name_equals(sql::_44)) // 44 == ascii(',')
      {
        switch (--n) {
          case 1: {
            if (name1)
              *name1 = subitem->value();
            break;
          }
          case 2: {
            if (name2)
              *name2 = subitem->value();
            break;
          }
          case 3: {
            name = subitem->value();
            if (name3)
              *name3 = name;
            break;
          }
        }
      }
    }

    if (obj.is_valid())
      set_obj_name(obj, name);
  }

  return name;
}

std::string Mysql_sql_parser::process_float_options_item(const SqlAstNode *item, std::string *precision,
                                                         std::string *scale) {
  std::string precision_ = "";

  if (precision)
    precision->clear();
  if (scale)
    scale->clear();

  if (item) {
    const SqlAstNode *scale_item = item->subitem(sql::_precision);
    if (scale_item) {
      const SqlAstNode *num_item;

      if ((num_item = scale_item->subitem(sql::_NUM)))
        precision_ = num_item->value();

      if (scale && ((num_item = scale_item->find_subseq(scale_item->subitem(sql::_44), sql::_NUM)))) // 44 == ascii(',')
        *scale = num_item->value();
    } else {
      const SqlAstNode *num_item;
      if ((num_item = item->subitem(sql::_NUM)))
        precision_ = num_item->value();
    }
  }

  if (precision)
    *precision = precision_;

  return precision_;
}

std::string Mysql_sql_parser::process_obj_full_name_item(const SqlAstNode *item, db_mysql_SchemaRef *schema) {
  std::string obj_name;
  std::string schema_name = "";

  if (!item)
    return obj_name;

  if (3 == item->subitems()->size()) // ident.ident
    schema_name = (*item->subitems()->begin())->value();
  obj_name = (*item->subitems()->rbegin())->value();

  // this will add schema to schemata if necessary
  db_mysql_SchemaRef schema_ = ensure_schema_created(schema_name, false);

  // if schema that object is sticked to doesn't correspond to the one pointed in DDL
  if (_stick_to_active_schema && (schema_ != _active_schema)) {
    // give the object a special name
    const char *spec_name_postfix = "_WRONG_SCHEMA";
    if (std::string::npos == obj_name.find(spec_name_postfix))
      obj_name.append(spec_name_postfix);

    // also substitute schema by active schema
    schema_ = _active_schema;
  }

  if (schema)
    *schema = schema_;

  return obj_name;
}

void Mysql_sql_parser::process_index_item(const SqlAstNode *tree, db_mysql_TableRef &table) {
  db_mysql_IndexRef obj(grt::Initialized);
  obj->owner(table);

  // name
  {
    static sql::symbol path1[] = {sql::_opt_ident, sql::_field_ident, sql::_};
    static sql::symbol path2[] = {sql::_opt_constraint, sql::_constraint, sql::_opt_ident, sql::_field_ident, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    const SqlAstNode *item = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    process_field_name_item(item, obj);
  }

  // index type
  {
    std::string index_type;

    static sql::symbol path1[] = {sql::_normal_key_type, sql::_};
    static sql::symbol path2[] = {sql::_fulltext, sql::_};
    static sql::symbol path3[] = {sql::_spatial, sql::_};
    static sql::symbol path4[] = {sql::_constraint_key_type, sql::_};
    static sql::symbol *paths[] = {path1, path2, path3, path4};

    const SqlAstNode *item = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (item) {
      if (item->subitem(sql::_PRIMARY_SYM)) {
        obj->isPrimary(1);
        table->primaryKey(obj);
        // if ((*obj->name()).empty()) // server resets any explicitly specified pk constraint names
        set_obj_name(obj, "PRIMARY");
        index_type = "PRIMARY";
      } else if (item->subitem(sql::_UNIQUE_SYM)) {
        obj->unique(1);
        index_type = "UNIQUE";
      } else
        index_type = item->restore_sql_text(_sql_statement);
    } else
      index_type = "INDEX";

    obj->indexType(shape_index_type(index_type));
  }

  // index kind
  process_index_kind_item(obj, tree->subitem(sql::_key_alg, sql::_key_using_alg, sql::_btree_or_rtree));

  // columns
  {
    const SqlAstNode *items = tree->subitem(sql::_key_list);
    if (items) {
      db_mysql_IndexColumnRef index_column(grt::Initialized);
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_key_part)) {
          index_column = db_mysql_IndexColumnRef(grt::Initialized);
          index_column->owner(obj);

          // column
          const SqlAstNode *name_item = item->subitem(sql::_ident);
          std::string column_name = (name_item ? name_item->value() : "");
          db_mysql_ColumnRef column = find_named_object_in_list(table->columns(), column_name,
                                                                false); // mysql columns are always case-insensitive
          if (!column.is_valid()) {
            std::string err_text;
            err_text.append("Column `").append(column_name).append("` not found");
            throw Parse_exception(err_text);
          }
          index_column->referencedColumn(column);

          // stored function
          // there is nothing relevant in separate 'CREATE' statement (v5.1)
          // index_column.storedFunction("");

          // stored function
          // there is nothing relevant in separate 'CREATE' statement (v5.1)
          // index_column.comment("");

          // length
          SET_INT_SI(index_column->columnLength, item, sql::_NUM);

          obj->columns().insert(index_column);
        } else if (item->name_equals(sql::_order_dir)) {
          // order direction
          index_column->descend(are_strings_eq_ci("DESC", item->value()) ? 1 : 0);
        }
      }
    }
  }

  // options
  process_index_options_item(obj, tree);

  table->indices().insert(obj);
}

void Mysql_sql_parser::process_fk_item(const SqlAstNode *tree, db_mysql_TableRef &table) {
  db_mysql_ForeignKeyRef obj(grt::Initialized);
  obj->owner(table);
  Fk_ref fk_ref(obj);

  // name
  {
    static sql::symbol path1[] = {sql::_opt_ident, sql::_field_ident, sql::_};
    static sql::symbol path2[] = {sql::_opt_constraint, sql::_constraint, sql::_opt_ident, sql::_field_ident, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    const SqlAstNode *item = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    process_field_name_item(item, obj);

    if (_gen_fk_names_when_empty && obj->name().operator std::string().empty()) {
      std::string name = bec::TableHelper::generate_foreign_key_name();
      set_obj_name(obj, name);
    }
  }

  // own columns
  {
    const SqlAstNode *items = tree->subitem(sql::_key_list);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           it++) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_key_part)) {
          const SqlAstNode *colname_item = item->subitem(sql::_ident);

          if (colname_item) {
            std::string colname = colname_item->value();

            db_mysql_ColumnRef column =
              find_named_object_in_list(table->columns(), colname, false); // mysql columns are always case-insensitive
            if (!column.is_valid()) {
              std::string err_text;
              err_text.append("Column `").append(colname).append("` not found");
              throw Parse_exception(err_text);
            } else
              obj->columns().insert(column);
          }
        }
      }
    }
  }

  // cardinality/mandatory
  {
    obj->referencedMandatory(1);
    grt::ListRef<db_Column> columns = obj->columns();
    for (size_t n = 0, count = columns.count(); n < count; ++n) {
      if (!columns.get(n)->isNotNull()) {
        obj->referencedMandatory(0);
        break;
      }
    }
  }
  obj->many(1);

  process_fk_references_item(tree->subitem(sql::_references), obj, fk_ref);

  table->foreignKeys().insert(obj);
  _fk_refs.push_back(fk_ref);
}

void Mysql_sql_parser::process_fk_references_item(const SqlAstNode *tree, db_mysql_ForeignKeyRef &fk, Fk_ref &fk_ref) {
  // tree is a 'references' item
  if (!tree)
    return;

  // referred table - use fk decorator to store names - set references at next stage
  db_mysql_SchemaRef ref_schema;
  db_mysql_TableRef ref_table;
  {
    // reset reuse_existing that could've been set for the owner table object, but would
    // prevent a stub schema to be created by process_obj_full_name_item() in case its necessary
    Val_keeper<bool> reusing_existing_objects_keeper(&_reusing_existing_obj);
    _reusing_existing_obj = false;

    std::string ref_obj_name = process_obj_full_name_item(tree->subitem(sql::_table_ident), &ref_schema);
    fk_ref.ref_schema_name(ref_schema->name());
    fk_ref.ref_table_name(ref_obj_name);
  }

  // ref columns names - use fk decorator to store names - set references at next stage
  {
    const SqlAstNode *items = tree->subitem(sql::_opt_ref_list, sql::_ref_list);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           it++) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_ident))
          fk_ref.ref_column_names().push_back(item->value());
      }
    }
  }

  // on update/delete rule
  if (const SqlAstNode *opt_on_update_delete_item = tree->subitem(sql::_opt_on_update_delete)) {
    if (const SqlAstNode *delete_option_item =
          opt_on_update_delete_item->find_subseq(sql::_DELETE_SYM, sql::_delete_option))
      fk->deleteRule(delete_option_item->restore_sql_text(_sql_statement));
    if (const SqlAstNode *delete_option_item =
          opt_on_update_delete_item->find_subseq(sql::_UPDATE_SYM, sql::_delete_option))
      fk->updateRule(delete_option_item->restore_sql_text(_sql_statement));
  }
}

void Mysql_sql_parser::process_index_options_item(db_mysql_IndexRef &obj, const SqlAstNode *item) {
  static sql::symbol path1[] = {sql::_normal_key_options, sql::_normal_key_opts, sql::_};
  static sql::symbol path2[] = {sql::_fulltext_key_options, sql::_fulltext_key_opts, sql::_};
  static sql::symbol path3[] = {sql::_spatial_key_options, sql::_spatial_key_opts, sql::_};
  static sql::symbol *paths[] = {path1, path2, path3};

  if (const SqlAstNode *items = item->search_by_paths(paths, ARR_CAPACITY(paths))) {
    for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
         ++it) {
      const SqlAstNode *item = *it;
      switch (item->name()) {
        case sql::_normal_key_opt:
        case sql::_fulltext_key_opt:
        case sql::_spatial_key_opt:
          if (const SqlAstNode *aux_item = item->subseq(sql::_key_using_alg))
            process_index_kind_item(obj, aux_item->subitem(sql::_btree_or_rtree));
          else if (/*const SqlAstNode *aux_item= */ item->subitem(sql::_all_key_opt, sql::_KEY_BLOCK_SIZE))
            SET_INT_SI(obj->keyBlockSize, item, sql::_all_key_opt, sql::_ulong_num)
          else if (item->subseq(sql::_WITH, sql::_PARSER_SYM))
            SET_STR_SI(obj->withParser, item, sql::_IDENT_sys)
          else if (item->subitem(sql::_all_key_opt, sql::_COMMENT_SYM))
            SET_STR_SI(obj->comment, item, sql::_all_key_opt, sql::_TEXT_STRING_sys)
          break;
        default:
          break;
      }
    }
  }
}

void Mysql_sql_parser::process_index_kind_item(db_mysql_IndexRef &obj, const SqlAstNode *item) {
  if (!item)
    return;

  const std::string index_kind = item->restore_sql_text(_sql_statement);
  if (!index_kind.empty())
    obj->indexKind(shape_index_kind(index_kind));
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_table_statement(const SqlAstNode *tree) {
  const SqlAstNode *create2_item = tree->subitem(sql::_create2);

  // check if statement is relevant
  if (!create2_item)
    return pr_irrelevant;

  ACTIVE_SCHEMA_KEEPER

  db_mysql_SchemaRef schema;
  db_mysql_TableRef obj;

  // try to find previously created stub object with the same name
  {
    const SqlAstNode *table_ident_item = tree->find_subseq(sql::_TABLE_SYM, sql::_table_ident);
    if (!table_ident_item)
      table_ident_item = tree->find_subseq(sql::_TABLE_SYM, sql::_opt_if_not_exists, sql::_table_ident);
    std::string obj_name = process_obj_full_name_item(table_ident_item, &schema);

    step_progress(obj_name);

    _active_schema = schema;

    // check for same-named view
    {
      db_mysql_ViewRef obj = find_named_object_in_list(schema->views(), obj_name, _case_sensitive_identifiers);
      if (obj.is_valid()) {
        Val_keeper<bool> reuse_existing_objects_keeper(&_reuse_existing_objects);
        _reuse_existing_objects = false;
        blame_existing_obj(true, obj, schema);
      }
    }

    {
      Val_keeper<bool> reuse_existing_objects_keeper(&_reuse_existing_objects);
      _reuse_existing_objects = true;
      obj = create_or_find_named_obj(schema->tables(), obj_name, _case_sensitive_identifiers, schema);
    }

    if (_reusing_existing_obj) {
      if (obj->isStub()) {
        obj->isStub(0); // reuse equally named stub

        std::string msg_text;
        msg_text.append("Previously created stub for table `")
          .append(*schema->name())
          .append("`.`")
          .append(obj_name)
          .append("` was found. Reusing.");
        add_log_message(msg_text, 1);
      } else
        blame_existing_obj(true, obj, schema);
    } else {
      // name
      std::string name = process_obj_full_name_item(table_ident_item, NULL);
      if (obj.is_valid())
        set_obj_name(obj, name);
    }
  }

  // options
  {
    const SqlAstNode *items =
      create2_item->subitem(sql::_create2a, sql::_opt_create_table_options, sql::_create_table_options);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_create_table_option)) {
          const SqlAstNode *aux_item = NULL;

          if (item->subseq(sql::_ENGINE_SYM) || item->subseq(sql::_TYPE_SYM)) {
            SET_STR_SI(obj->tableEngine, item, sql::_storage_engines)
            obj->tableEngine(__table_storage_engines.normalize_name(obj->tableEngine()));
          } else if (item->subseq(sql::_ROW_FORMAT_SYM))
            SET_STR_SI(obj->rowFormat, item, sql::_row_types)
          else if (item->subseq(sql::_KEY_BLOCK_SIZE))
            SET_STR_SI(obj->keyBlockSize, item, sql::_ulong_num)
          else if (item->subseq(sql::_AUTO_INC))
            SET_STR_SI(obj->nextAutoInc, item, sql::_ulonglong_num)
          else if ((aux_item = item->subseq(sql::_default_charset)))
            SET_STR_SI(cs_collation_setter(obj, schema, false).charset_name, aux_item, sql::_charset_name_or_default)
          else if ((aux_item = item->subseq(sql::_default_collation)))
            SET_STR_SI(cs_collation_setter(obj, schema, false).collation_name, aux_item,
                       sql::_collation_name_or_default)
          else if (item->subseq(sql::_DELAY_KEY_WRITE_SYM))
            SET_INT_SI(obj->delayKeyWrite, item, sql::_ulong_num)
          else if (item->subseq(sql::_COMMENT_SYM))
            SET_STR_SI(obj->comment, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_DATA_SYM, sql::_DIRECTORY_SYM))
            SET_STR_SI(obj->tableDataDir, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_INDEX_SYM, sql::_DIRECTORY_SYM))
            SET_STR_SI(obj->tableIndexDir, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_PACK_KEYS_SYM)) {
            SET_STR_SI(obj->packKeys, item, sql::_DEFAULT)
            SET_STR_SI(obj->packKeys, item, sql::_ulong_num)
          } else if (item->subseq(sql::_AVG_ROW_LENGTH))
            SET_STR_SI(obj->avgRowLength, item, sql::_ulong_num)
          else if (item->subseq(sql::_MIN_ROWS))
            SET_STR_SI(obj->minRows, item, sql::_ulonglong_num)
          else if (item->subseq(sql::_MAX_ROWS))
            SET_STR_SI(obj->maxRows, item, sql::_ulonglong_num)
          else if (item->subseq(sql::_CHECKSUM_SYM))
            SET_INT_SI(obj->checksum, item, sql::_ulong_num)
          else if (item->subseq(sql::_INSERT_METHOD))
            SET_STR_SI(obj->mergeInsert, item, sql::_merge_insert_types)
          else if (item->subseq(sql::_UNION_SYM)) {
            /*
              server cuts schema qualification from table names (only for those located within the same schema with
              table being defined)
              in union list when using 'show create' statement.
              that's why to avoid false differences during merge procedure, all table names are explicitly normalized
              (fully qualified names only).
            */
            const SqlAstNode *items = item->subitem(sql::_opt_table_list, sql::_table_list);
            if (items && items->subitems()) {
              std::string table_list;
              for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin();
                   it != items->subitems()->end(); ++it) {
                const SqlAstNode *item = *it;
                if (item->name_equals(sql::_table_name)) {
                  db_mysql_SchemaRef schema;
                  std::string obj_name = process_obj_full_name_item(item->subitem(sql::_table_ident), &schema);
                  if (!table_list.empty())
                    table_list.append(", ");
                  table_list.append(qualify_obj_name(obj_name, schema->name()));
                }
              }
              if (!table_list.empty())
                obj->mergeUnion(table_list);
            }
          }
        }
      }
    }
  }

  // partitioning
  {
    class Partition_definition {
      static void parse_options(db_mysql_PartitionDefinitionRef part_obj, const SqlAstNode *part_options) {
        for (SqlAstNode::SubItemList::const_iterator it = part_options->subitems()->begin(),
                                                     it_end = part_options->subitems()->end();
             it != it_end; ++it) {
          const SqlAstNode *part_option = *it;
          if (!part_option->name_equals(sql::_opt_part_option))
            continue;

          const SqlAstNode *part_option_name;

          if ((part_option_name = part_option->subitem(sql::_MAX_ROWS)))
            SET_STR_SI(part_obj->maxRows, part_option, sql::_real_ulonglong_num)
          else if ((part_option_name = part_option->subitem(sql::_MIN_ROWS)))
            SET_STR_SI(part_obj->minRows, part_option, sql::_real_ulonglong_num)
          else if ((part_option_name = part_option->subitem(sql::_DATA_SYM)))
            SET_STR_SI(part_obj->dataDirectory, part_option, sql::_TEXT_STRING_sys)
          else if ((part_option_name = part_option->subitem(sql::_INDEX_SYM)))
            SET_STR_SI(part_obj->indexDirectory, part_option, sql::_TEXT_STRING_sys)
          else if ((part_option_name = part_option->subitem(sql::_COMMENT_SYM)))
            SET_STR_SI(part_obj->comment, part_option, sql::_TEXT_STRING_sys)
          // TODO: process ENGINE (not supported by server as of 5.1.22)
        }
      }

      // returns count of subpartitions
      static void parse_subpartitions(db_mysql_PartitionDefinitionRef part_obj, const SqlAstNode *subpart_list) {
        for (SqlAstNode::SubItemList::const_iterator it = subpart_list->subitems()->begin(),
                                                     it_end = subpart_list->subitems()->end();
             it != it_end; ++it) {
          const SqlAstNode *subpart_item = *it;
          if (!subpart_item->name_equals(sql::_sub_part_definition))
            continue;

          db_mysql_PartitionDefinitionRef subpart_obj(grt::Initialized);

          SET_STR_SI(subpart_obj->name, subpart_item, sql::_sub_name);
          parse_options(subpart_obj, subpart_item->subitem(sql::_opt_part_options, sql::_opt_part_option_list));

          part_obj->subpartitionDefinitions().insert(subpart_obj);
        }
      }

    public:
      static db_mysql_PartitionDefinitionRef parse(const SqlAstNode *part_item, const std::string &_sql_statement) {
        db_mysql_PartitionDefinitionRef part_obj(grt::Initialized);

        const SqlAstNode *part_attr;

        if ((part_attr = part_item->subitem(sql::_part_name)))
          part_obj->name(part_attr->value());

        if (const SqlAstNode *opt_part_values_item = part_item->subitem(sql::_opt_part_values)) {
          static sql::symbol path1[] = {sql::_part_func_max, sql::_MAX_VALUE_SYM, sql::_};
          static sql::symbol path2[] = {sql::_part_func_max, sql::_part_value_item, sql::_part_value_item_list, sql::_};
          static sql::symbol path3[] = {sql::_part_values_in, sql::_part_value_item, sql::_part_value_item_list,
                                        sql::_};
          static sql::symbol path4[] = {sql::_part_values_in, sql::_part_value_list, sql::_};
          static sql::symbol *paths[] = {path1, path2, path3, path4};

          part_attr = opt_part_values_item->search_by_paths(paths, ARR_CAPACITY(paths));

          SET_SQL_I(part_obj->value, part_attr)
        }

        if ((part_attr = part_item->subitem(sql::_opt_part_options, sql::_opt_part_option_list)))
          parse_options(part_obj, part_attr);

        if ((part_attr = part_item->subitem(sql::_opt_sub_partition, sql::_sub_part_list)))
          parse_subpartitions(part_obj, part_attr);

        return part_obj;
      }
    }; // class Partition_definition

    // clear partition list in case this is a reused table
    obj->partitionDefinitions().remove_all();

    const SqlAstNode *partition =
      create2_item->subitem(sql::_create2a, sql::_opt_create_partitioning, sql::_partitioning, sql::_partition);
    if (partition) {
      bool is_range_or_list_partition = (partition->subitem(sql::_part_type_def, sql::_RANGE_SYM) != NULL) ||
                                        (partition->subitem(sql::_part_type_def, sql::_LIST_SYM) != NULL);

      const SqlAstNode *item;

      if ((item = partition->subitem(sql::_part_type_def))) {
        std::string part_type;
        const SqlAstNode *part_exprt_item = NULL;

        if (item->subitem(sql::_LINEAR_SYM))
          part_type.append("LINEAR ");

        if (item->subitem(sql::_HASH_SYM))
          part_type.append("HASH");
        else if (item->subitem(sql::_KEY_SYM)) {
          part_type.append("KEY");
          part_exprt_item = item->subitem(sql::_part_field_list, sql::_part_field_item_list);
        } else if (item->subitem(sql::_RANGE_SYM))
          part_type.append("RANGE");
        else if (item->subitem(sql::_LIST_SYM))
          part_type.append("LIST");

        obj->partitionType(part_type);

        if (!part_exprt_item)
          part_exprt_item = item->subitem(sql::_part_func, sql::_part_func_expr);

        SET_SQL_I(obj->partitionExpression, part_exprt_item);
      }

      if ((item = partition->subitem(sql::_opt_num_parts))) {
        if (!is_range_or_list_partition) // is not valid for range partitions
          SET_INT_SI(obj->partitionCount, item, sql::_real_ulong_num);
      }

      if ((item = partition->subitem(sql::_opt_sub_part))) {
        if (is_range_or_list_partition) // valid only for range partitions
        {
          std::string subpart_type;

          if (item->subitem(sql::_LINEAR_SYM))
            subpart_type.append("LINEAR ");

          if (item->subitem(sql::_HASH_SYM))
            subpart_type.append("HASH");
          else if (item->subitem(sql::_KEY_SYM))
            subpart_type.append("KEY");

          obj->subpartitionType(subpart_type);

          SET_SQL_I(obj->subpartitionExpression, item->subitem(sql::_sub_part_func, sql::_part_func_expr));
        }
      }

      if ((item = partition->subitem(sql::_part_defs))) {
        if (is_range_or_list_partition) // valid only for range partitions
        {
          const SqlAstNode *part_def_list = item->subitem(sql::_part_def_list);

          for (SqlAstNode::SubItemList::const_iterator jt = part_def_list->subitems()->begin(),
                                                       jt_end = part_def_list->subitems()->end();
               jt != jt_end; ++jt) {
            const SqlAstNode *part = *jt;
            if (part->name_equals(sql::_part_definition)) {
              db_mysql_PartitionDefinitionRef part_obj = Partition_definition::parse(part, _sql_statement);
              obj->partitionDefinitions().insert(part_obj);
            }
          }
        }
      }

      if (obj->partitionCount() == 0)
        obj->partitionCount((long)obj->partitionDefinitions().count());
      if (obj->partitionDefinitions().count() > 0)
        obj->subpartitionCount((long)obj->partitionDefinitions().get(0)->subpartitionDefinitions().count());
    }
  }

  // columns, indices, foreign keys
  {
    const SqlAstNode *items = create2_item->subitem(sql::_create2a, sql::_create_field_list, sql::_field_list);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_field_list_item)) {
          const SqlAstNode *aux_item = NULL;

          if ((aux_item = item->subitem(sql::_column_def))) {
            db_mysql_ColumnRef column;

            // field
            if ((aux_item = aux_item->subitem(sql::_field_spec))) {
              bool reusing_column = false;

              // if current table is reusing stub object then try to find existing column with the same name
              if (_reusing_existing_obj) {
                std::string field_name = process_field_name_item(aux_item->subitem(sql::_field_ident));
                column = find_named_object_in_list(obj->columns(), field_name,
                                                   false); // mysql columns are always case-insensitive
              }

              if (column.is_valid())
                reusing_column = true;
              else {
                column = db_mysql_ColumnRef(grt::Initialized);
                column->owner(obj);
              }

              // name
              process_field_name_item(aux_item->subitem(sql::_field_ident), column);

              // type
              process_field_type_item(aux_item->subitem(sql::_type), column);

              // attributes
              process_field_attributes_item(aux_item->subitem(sql::_opt_attribute, sql::_opt_attribute_list), column,
                                            obj);

              if (!reusing_column)
                obj->columns().insert(column);
            }

            if ((aux_item = item->subitem(sql::_column_def, sql::_references))) {
              db_mysql_ForeignKeyRef fk(grt::Initialized);
              fk->owner(obj);
              Fk_ref fk_ref(fk);

              // own columns
              fk->columns().insert(column);

              // name
              if (_gen_fk_names_when_empty) {
                std::string name = bec::TableHelper::generate_foreign_key_name();
                set_obj_name(fk, name);
              }

              // cardinality/mandatory
              fk->referencedMandatory(1);
              grt::ListRef<db_Column> columns = fk->columns();
              for (size_t n = 0, count = columns.count(); n < count; ++n) {
                if (!columns.get(n)->isNotNull()) {
                  fk->referencedMandatory(0);
                  break;
                }
              }
              fk->many(1);

              // references
              process_fk_references_item(aux_item, fk, fk_ref);

              obj->foreignKeys().insert(fk);
              _fk_refs.push_back(fk_ref);
            }
          } else if ((aux_item = item->subitem(sql::_key_def))) {
            if ((aux_item->find_subseq(sql::_FOREIGN, sql::_KEY_SYM)))
              ENSURE(process_fk_item(aux_item, obj), "Table `" + *obj->name() + "` : " + e.what())
            else if ((aux_item->subitem(sql::_key_list)))
              ENSURE(process_index_item(aux_item, obj), "Table `" + *obj->name() + "` : " + e.what())
          }
        }
      }
    }
  }

  if (_shape_table)
    _shape_table(obj);
  do_transactable_list_insert(schema->tables(), obj);
  if (!obj->isStub())
    log_db_obj_created(schema, obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_view_statement(const SqlAstNode *tree) {
  const SqlAstNode *view_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol path3[] = {sql::_view_or_trigger_or_sp_or_event, sql::_};
    static sql::symbol *paths[] = {path1, path2, path3};

    view_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (view_tail)
      view_tail = view_tail->subitem(sql::_view_tail);
  }

  // check if statement is relevant
  if (!view_tail)
    return pr_irrelevant;

  db_mysql_SchemaRef schema;
  const SqlAstNode *table_ident_item = view_tail->find_subseq(sql::_VIEW_SYM, sql::_table_ident);
  std::string obj_name = process_obj_full_name_item(table_ident_item, &schema);
  step_progress(obj_name);

  // check for same-named table
  {
    db_mysql_TableRef obj = find_named_object_in_list(schema->tables(), obj_name, _case_sensitive_identifiers);
    if (obj.is_valid()) {
      Val_keeper<bool> reuse_existing_objects_keeper(&_reuse_existing_objects);
      _reuse_existing_objects = false;
      blame_existing_obj(true, obj, schema);
    }
  }

  db_mysql_ViewRef obj = create_or_find_named_obj(schema->views(), obj_name, _case_sensitive_identifiers, schema);
  SET_SQL_SI(obj->definer, tree, sql::_view_or_trigger_or_sp_or_event, sql::_definer_opt, sql::_definer, sql::_user)

  const SqlAstNode *algorithm_node =
    tree->subitem(sql::_view_or_trigger_or_sp_or_event, sql::_view_replace_or_algorithm, sql::_view_algorithm);
  int algorithm = 0;
  if (algorithm_node == nullptr || algorithm_node->subitem(sql::_UNDEFINED_SYM))
    algorithm = 0;
  else if (algorithm_node->subitem(sql::_MERGE_SYM))
    algorithm = 1;
  else if (algorithm_node->subitem(sql::_TEMPTABLE_SYM))
    algorithm = 2;
  obj->algorithm(algorithm);

  const SqlAstNode *select_node = view_tail->find_subseq(sql::_view_select);
  std::string select;
  if (select_node)
    select = select_node->restore_sql_text(_sql_statement);
  obj->sqlBody(select);

  // name
  std::string name = process_obj_full_name_item(table_ident_item, NULL);
  if (obj.is_valid())
    set_obj_name(obj, name);

  // sql_statement
  set_obj_sql_def(obj);

  // check option
  if (view_tail->subitem(sql::_view_check_option))
    obj->withCheckCondition(1);

  _shape_view(obj);
  do_transactable_list_insert(schema->views(), obj);
  log_db_obj_created(schema, obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_routine_statement(const SqlAstNode *tree) {
  const SqlAstNode *routine_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    routine_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
  }
  if (routine_tail) {
    static sql::symbol path1[] = {sql::_sp_tail, sql::_};
    static sql::symbol path2[] = {sql::_sf_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    routine_tail = routine_tail->search_by_paths(paths, ARR_CAPACITY(paths));
  }

  // check if statement is relevant
  if (!routine_tail)
    return pr_irrelevant;

  std::string routine_type;
  if (routine_tail->subitem(sql::_FUNCTION_SYM))
    routine_type = "function";
  else if (routine_tail->subitem(sql::_PROCEDURE_SYM))
    routine_type = "procedure";

  db_mysql_SchemaRef schema;
  const SqlAstNode *routine_ident_item = routine_tail->subitem(sql::_sp_name);
  std::string obj_name = process_obj_full_name_item(routine_ident_item, &schema);
  step_progress(obj_name);

  db_mysql_RoutineRef obj = create_or_find_named_routine(schema->routines(), obj_name, false, routine_type,
                                                         schema); // mysql routines are always case-insensitive

  SET_SQL_SI(obj->definer, tree, sql::_view_or_trigger_or_sp_or_event, sql::_definer, sql::_user)
  std::string sql_body;
  const SqlAstNode *sql_body_node = routine_tail->find_subseq(sql::_sp_proc_stmt);
  if (sql_body_node)
    obj->sqlBody(sql_body_node->restore_sql_text(_sql_statement));

  // name
  std::string name = process_obj_full_name_item(routine_ident_item, NULL);
  if (obj.is_valid())
    set_obj_name(obj, name);

  {
    const SqlAstNode *param_list_item = NULL;

    obj->routineType(routine_type);
    if (routine_type == "procedure") {
      param_list_item = routine_tail->subitem(sql::_sp_pdparam_list, sql::_sp_pdparams);
    } else if (routine_type == "function") {
      param_list_item = routine_tail->subitem(sql::_sp_fdparam_list, sql::_sp_fdparams);

      // return datatype
      SET_SQL_SI(obj->returnDatatype, routine_tail, sql::_type_with_opt_collate, sql::_type)
    }

    // params
    {
      ListRef<db_mysql_RoutineParam> params = obj->params();

      if (_reusing_existing_obj)
        for (ssize_t n = params.count(); n > 0; --n)
          params.remove_value(params[n - 1]);

      if (param_list_item) {
        for (SqlAstNode::SubItemList::const_iterator it = param_list_item->subitems()->begin();
             it != param_list_item->subitems()->end(); ++it) {
          const SqlAstNode *item = *it;
          if (item->name_equals(sql::_sp_pdparam) || item->name_equals(sql::_sp_fdparam)) {
            db_mysql_RoutineParamRef param(grt::Initialized);
            param->owner(obj);

            SET_STR_SI(param->name, item, sql::_ident)
            SET_STR_SI(param->paramType, item, sql::_sp_opt_inout)

            // datatype
            SET_SQL_SI(param->datatype, item, sql::_type_with_opt_collate, sql::_type)

            params.insert(param);
          }
        }
      }
    }
  }

  // comment
  {
    const SqlAstNode *sp_c_chistics = routine_tail->subitem(sql::_sp_c_chistics);
    if (sp_c_chistics) {
      for (SqlAstNode::SubItemList::const_iterator it = sp_c_chistics->subitems()->begin(),
                                                   it_end = sp_c_chistics->subitems()->end();
           it != it_end; ++it) {
        const SqlAstNode *item = *it;
        if ((item = item->subitem(sql::_sp_chistic)) &&
            (item = item->find_subseq(sql::_COMMENT_SYM, sql::_TEXT_STRING_sys))) {
          SET_STR_I(obj->comment, item)
          break;
        }
      }
    }
  }

  // sql_statement
  set_obj_sql_def(obj);

  _shape_routine(obj);
  do_transactable_list_insert(schema->routines(), obj);
  log_db_obj_created(schema, obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_index_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->find_subseq(sql::_INDEX_SYM, sql::_ident))
    return pr_irrelevant;

  // table
  db_mysql_SchemaRef schema;
  db_mysql_TableRef table;
  {
    // table + schema
    std::string table_name = process_obj_full_name_item(tree->subitem(sql::_table_ident), &schema);

    table = find_named_object_in_list(schema->tables(), table_name, _case_sensitive_identifiers);
    if (!table.is_valid()) {
      {
        std::string msg_text;
        msg_text.append("Table `")
          .append(*schema->name())
          .append("`.`")
          .append(table_name)
          .append("` not found. Stub was created.");
        add_log_message(msg_text, 1);
      }
      create_stub_table(schema, table, table_name);
    }
  }

  const SqlAstNode *ident_item = tree->find_subseq(sql::_INDEX_SYM, sql::_ident);
  std::string obj_name = (ident_item) ? ident_item->value() : "";
  step_progress(obj_name);
  db_mysql_IndexRef obj = create_or_find_named_obj(table->indices(), obj_name, false, schema,
                                                   table); // mysql indexes are always case-insensitive

  set_obj_name(obj, obj_name);

  // is primary
  // primary index can't be created by separate 'CREATE' statement (v5.1)
  // obj->isPrimary(0);

  // deferability
  // there is nothing relevant in separate 'CREATE' statement (v5.1)
  // obj->deferability(0);

  // index type
  {
    static sql::symbol names[] = {sql::_opt_unique, sql::_fulltext, sql::_spatial};
    std::string index_type;
    if (const SqlAstNode *item = tree->search_by_names(names, ARR_CAPACITY(names))) {
      index_type = item->restore_sql_text(_sql_statement);
      if (item->name_equals(sql::_opt_unique))
        obj->unique(1);
    } else {
      index_type = "INDEX";
    }
    obj->indexType(shape_index_type(index_type));
  }

  // index kind
  process_index_kind_item(obj, tree->subitem(sql::_key_alg, sql::_key_using_alg, sql::_btree_or_rtree));

  // columns
  {
    const SqlAstNode *items = tree->subitem(sql::_key_list);
    if (items) {
      db_mysql_IndexColumnRef index_column(grt::Initialized);
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_key_part)) {
          index_column = db_mysql_IndexColumnRef(grt::Initialized);
          index_column->owner(obj);

          // referred column
          const SqlAstNode *name_item = item->subitem(sql::_ident);
          std::string column_name = (name_item ? name_item->value() : "");
          db_mysql_ColumnRef column = find_named_object_in_list(table->columns(), column_name,
                                                                false); // mysql columns are always case-insensitive
          if (!column.is_valid()) {
            std::string err_text;
            err_text.append("Column `").append(column_name).append("` not found");
            throw Parse_exception(err_text);
          }
          index_column->referencedColumn(column);

          // stored function
          // there is nothing relevant in separate 'CREATE' statement (v5.1)
          // index_column.storedFunction("");

          // stored function
          // there is nothing relevant in separate 'CREATE' statement (v5.1)
          // index_column.comment("");

          // length
          SET_INT_SI(index_column->columnLength, item, sql::_NUM);

          obj->columns().insert(index_column);
        } else if (item->name_equals(sql::_order_dir)) {
          // order direction
          index_column->descend(are_strings_eq_ci("DESC", item->value()) ? 1 : 0);
        }
      }
    }
  }

  // options
  process_index_options_item(obj, tree);

  _shape_index(obj);
  do_transactable_list_insert(table->indices(), obj);
  log_db_obj_created(schema, table, obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_logfile_group_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_LOGFILE_SYM, sql::_GROUP_SYM))
    return pr_irrelevant;

  const SqlAstNode *info_item = tree->subitem(sql::_logfile_group_info);
  const SqlAstNode *ident_item = info_item->subitem(sql::_logfile_group_name);
  std::string obj_name = (ident_item) ? ident_item->value() : "";
  step_progress(obj_name);
  db_mysql_LogFileGroupRef obj =
    create_or_find_named_obj(_catalog->logFileGroups(), obj_name, _case_sensitive_identifiers);

  set_obj_name(obj, obj_name);

  // undofile
  SET_STR_SI(obj->undoFile, info_item, sql::_add_log_file, sql::_lg_undofile, sql::_TEXT_STRING_sys);

  // options
  {
    const SqlAstNode *items = info_item->subitem(sql::_logfile_group_option_list, sql::_logfile_group_options);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_logfile_group_option)) {
          const SqlAstNode *aux_item;
          if ((aux_item = item->subitem(sql::_opt_ts_initial_size)))
            SET_INT_SI(obj->initialSize, aux_item, sql::_size_number)
          else if ((aux_item = item->subitem(sql::_opt_ts_undo_buffer_size)))
            SET_INT_SI(obj->undoBufferSize, aux_item, sql::_size_number)
          else if ((aux_item = item->subitem(sql::_opt_ts_engine)))
            SET_STR_SI(obj->engine, aux_item, sql::_storage_engines)
        }
      }
    }
  }

  _shape_logfile_group(obj);
  do_transactable_list_insert(_catalog->logFileGroups(), obj);
  log_db_obj_created(obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_tablespace_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_TABLESPACE))
    return pr_irrelevant;

  const SqlAstNode *info_item = tree->subitem(sql::_tablespace_info);
  const SqlAstNode *ident_item = info_item->subitem(sql::_tablespace_name);
  std::string obj_name = (ident_item) ? ident_item->value() : "";
  step_progress(obj_name);
  db_mysql_TablespaceRef obj = create_or_find_named_obj(_catalog->tablespaces(), obj_name, _case_sensitive_identifiers);

  set_obj_name(obj, obj_name);

  // datafile
  SET_STR_SI(obj->dataFile, info_item, sql::_ts_datafile, sql::_TEXT_STRING_sys);

  // logfile group
  {
    std::string logfile_group_name = get_str_attr_from_subitem(info_item, sql::_opt_logfile_group_name, sql::_ident);
    db_mysql_LogFileGroupRef logfile_group =
      find_named_object_in_list(_catalog->logFileGroups(), logfile_group_name, _case_sensitive_identifiers);
    if (!logfile_group.is_valid()) {
      std::string err_text;
      err_text.append("Log file group `").append(logfile_group_name).append("` not found");
      throw Parse_exception(err_text);
    }
    obj->logFileGroup(logfile_group);
  }

  // options
  {
    const SqlAstNode *items = info_item->subitem(sql::_tablespace_option_list, sql::_tablespace_options);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_tablespace_option)) {
          const SqlAstNode *aux_item;
          if ((aux_item = item->subitem(sql::_opt_ts_initial_size)))
            SET_INT_SI(obj->initialSize, aux_item, sql::_size_number)
          else if ((aux_item = item->subitem(sql::_opt_ts_extent_size)))
            SET_INT_SI(obj->extentSize, aux_item, sql::_size_number)
          else if ((aux_item = item->subitem(sql::_opt_ts_engine)))
            SET_STR_SI(obj->engine, aux_item, sql::_storage_engines)
        }
      }
    }
  }

  _shape_tablespace(obj);
  do_transactable_list_insert(_catalog->tablespaces(), obj);
  log_db_obj_created(obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_server_link_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_server_def))
    return pr_irrelevant;

  const SqlAstNode *server_def_item = tree->subitem(sql::_server_def);
  const SqlAstNode *ident_item = server_def_item->find_subseq(sql::_SERVER_SYM, sql::_ident_or_text);
  std::string obj_name = (ident_item) ? ident_item->value() : "";
  step_progress(obj_name);
  db_mysql_ServerLinkRef obj = create_or_find_named_obj(_catalog->serverLinks(), obj_name, _case_sensitive_identifiers);

  set_obj_name(obj, obj_name);

  // foreign data wrapper
  SET_STR_I(obj->wrapperName,
            server_def_item->find_subseq(sql::_FOREIGN, sql::_DATA_SYM, sql::_WRAPPER_SYM, sql::_ident_or_text))

  // options
  {
    const SqlAstNode *items = server_def_item->subitem(sql::_server_options_list);
    if (items) {
      for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
           ++it) {
        const SqlAstNode *item = *it;
        if (item->name_equals(sql::_server_option)) {
          if (item->subseq(sql::_HOST_SYM))
            SET_STR_SI(obj->host, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_DATABASE))
            SET_STR_SI(obj->schema, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_USER))
            SET_STR_SI(obj->user, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_PASSWORD))
            SET_STR_SI(obj->password, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_SOCKET_SYM))
            SET_STR_SI(obj->socket, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_OWNER_SYM))
            SET_STR_SI(obj->ownerUser, item, sql::_TEXT_STRING_sys)
          else if (item->subseq(sql::_PORT_SYM))
            SET_STR_SI(obj->port, item, sql::_ulong_num)
        }
      }
    }
  }

  _shape_serverlink(obj);
  do_transactable_list_insert(_catalog->serverLinks(), obj);
  log_db_obj_created(obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_trigger_statement(const SqlAstNode *tree) {
  const SqlAstNode *trigger_tail = NULL;
  {
    static sql::symbol path1[] = {sql::_view_or_trigger_or_sp_or_event, sql::_definer_tail, sql::_};
    static sql::symbol path2[] = {sql::_view_or_trigger_or_sp_or_event, sql::_no_definer_tail, sql::_};
    static sql::symbol *paths[] = {path1, path2};

    trigger_tail = tree->search_by_paths(paths, ARR_CAPACITY(paths));
    if (trigger_tail)
      trigger_tail = trigger_tail->subitem(sql::_trigger_tail);
  }

  // check if statement is relevant
  if (!trigger_tail || !trigger_tail->subseq(sql::_TRIGGER_SYM))
    return pr_irrelevant;

  // table + schema
  db_mysql_SchemaRef schema;
  db_mysql_TableRef table;
  {
    const SqlAstNode *table_ident = trigger_tail->subitem(sql::_table_ident);
    std::string table_name;
    {
      std::string schema_name;
      Mysql_sql_parser_base::process_obj_full_name_item(table_ident, schema_name, table_name);
    }

    if (_triggers_owner_table.is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(_triggers_owner_table->owner());
      table = _triggers_owner_table;
    } else {
      process_obj_full_name_item(table_ident, &schema);
      table = find_named_object_in_list(schema->tables(), table_name, _case_sensitive_identifiers);
    }

    if (!table.is_valid()) {
      {
        std::string msg_text;
        msg_text.append("Table `")
          .append(*schema->name())
          .append("`.`")
          .append(table_name)
          .append("` not found. Stub was created.");
        add_log_message(msg_text, 1);
      }
      create_stub_table(schema, table, table_name);
    }
  }

  const SqlAstNode *ident_item = trigger_tail->subitem(sql::_sp_name);
  std::string obj_name = process_obj_full_name_item(ident_item, NULL);
  step_progress(obj_name);
  db_mysql_TriggerRef obj =
    create_or_find_named_obj(table->triggers(), obj_name, _case_sensitive_identifiers, schema, table);

  // name
  std::string name = process_obj_full_name_item(ident_item, NULL);
  if (obj.is_valid())
    set_obj_name(obj, name);

  const SqlAstNode *body_tail = trigger_tail->subitem(sql::_sp_proc_stmt);
  if (body_tail)
    obj->sqlBody(body_tail->restore_sql_text(_sql_statement));

  // definer
  SET_SQL_SI(obj->definer, tree, sql::_view_or_trigger_or_sp_or_event, sql::_definer, sql::_user)

  // timing
  SET_STR_SI(obj->timing, trigger_tail, sql::_trg_action_time)

  // event
  SET_STR_SI(obj->event, trigger_tail, sql::_trg_event)

  /*
  // orientation
  if (trigger_tail->find_subseq(sql::_FOR_SYM, sql::_EACH_SYM, sql::_ROW_SYM))
    obj->orientation("ROW");
    */

  // enabled
  obj->enabled(1);

  // sql_statement
  set_obj_sql_def(obj);

  _shape_trigger(obj);
  do_transactable_list_insert(table->triggers(), obj);
  log_db_obj_created(schema, table, obj);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_create_schema_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_CREATE, sql::_DATABASE))
    return pr_irrelevant;

  const SqlAstNode *item = tree->subitem(sql::_ident);

  if (!item)
    throw Parse_exception("Invalid 'create database' statement");

  step_progress(item->value());

  db_mysql_SchemaRef schema = ensure_schema_created(item->value(), true);
  if (schema.is_valid()) {
    // options
    {
      const SqlAstNode *items = tree->subitem(sql::_opt_create_database_options, sql::_create_database_options);
      if (items) {
        for (SqlAstNode::SubItemList::const_iterator it = items->subitems()->begin(); it != items->subitems()->end();
             ++it) {
          const SqlAstNode *item = *it;
          if (item->name_equals(sql::_create_database_option)) {
            if (const SqlAstNode *aux_item = item->subitem(sql::_default_charset, sql::_charset_name_or_default))
              SET_STR_I(cs_collation_setter(db_SchemaRef(schema), db_CatalogRef(_catalog), true).charset_name, aux_item)
            else if (const SqlAstNode *aux_item =
                       item->subitem(sql::_default_collation, sql::_collation_name_or_default))
              SET_STR_I(cs_collation_setter(db_SchemaRef(schema), db_CatalogRef(_catalog), true).collation_name,
                        aux_item)
          }
        }
      }
    }
  }

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_drop_schema_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_DROP, sql::_DATABASE))
    return pr_irrelevant;

  bool if_exists = (NULL != tree->subitem(sql::_if_exists));

  const SqlAstNode *item = tree->subitem(sql::_ident);

  if (!item)
    throw Parse_exception("Invalid 'create database' statement");

  std::string obj_name = item->value();
  step_progress(obj_name);
  drop_obj(_catalog->schemata(), obj_name, if_exists);

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_drop_table_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subitem(sql::_table_or_tables))
    return pr_irrelevant;

  bool if_exists = (NULL != tree->subitem(sql::_if_exists));

  db_mysql_SchemaRef schema;
  const SqlAstNode *table_list = tree->subitem(sql::_table_list);
  for (SqlAstNode::SubItemList::const_iterator it = table_list->subitems()->begin(),
                                               it_end = table_list->subitems()->end();
       it != it_end; ++it) {
    const SqlAstNode *item = *it;
    if (!item->name_equals(sql::_table_name))
      continue;
    item = item->subitem(sql::_table_ident);
    std::string obj_name = process_obj_full_name_item(item, &schema);
    step_progress(obj_name);
    drop_obj(schema->tables(), obj_name, if_exists, schema);
  }

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_drop_view_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_DROP, sql::_VIEW_SYM))
    return pr_irrelevant;

  bool if_exists = (NULL != tree->subitem(sql::_if_exists));

  db_mysql_SchemaRef schema;
  const SqlAstNode *table_list = tree->subitem(sql::_table_list);
  for (SqlAstNode::SubItemList::const_iterator it = table_list->subitems()->begin(),
                                               it_end = table_list->subitems()->end();
       it != it_end; ++it) {
    const SqlAstNode *item = *it;
    if (!item->name_equals(sql::_table_name))
      continue;
    item = item->subitem(sql::_table_ident);
    std::string obj_name = process_obj_full_name_item(item, &schema);
    step_progress(obj_name);
    drop_obj(schema->views(), obj_name, if_exists, schema);
  }

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_drop_routine_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_DROP, sql::_FUNCTION_SYM) || !tree->subseq(sql::_DROP, sql::_PROCEDURE_SYM))
    return pr_irrelevant;
  /*
    bool if_exists= (NULL != tree->subitem(sql::_if_exists));

    db_mysql_SchemaRef schema;
    const SqlAstNode *ident_item= tree->subitem(sql::_sp_name);
    std::string obj_name= process_obj_full_name_item(ident_item, &schema);
    step_progress(obj_name);

    drop_obj(schema->routines(), obj_name, if_exists, &schema);
  */
  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_drop_trigger_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_DROP, sql::_TRIGGER_SYM))
    return pr_irrelevant;
  /*
    bool if_exists= (NULL != tree->subitem(sql::_if_exists));

    db_mysql_SchemaRef schema;
    const SqlAstNode *ident_item= tree->subitem(sql::_sp_name);
    std::string obj_name= process_obj_full_name_item(ident_item, &schema);
    step_progress(obj_name);

    //! TODO:
    //! search in tables for trigger to be dropped.
    //! delete trigger object from app tree.
    drop_obj(schema->routines(), obj_name, if_exists, &schema, &table);
  */
  return pr_processed;
}

template <typename T>
bool Mysql_sql_parser::drop_obj(grt::ListRef<T> obj_list, const std::string &obj_name, bool if_exists,
                                GrtNamedObjectRef owner, GrtNamedObjectRef grand_owner) {
  grt::Ref<T> obj = find_named_object_in_list(obj_list, obj_name, _case_sensitive_identifiers);
  if (obj.is_valid()) {
    GrtNamedObjectRef obj1 = grand_owner;
    GrtNamedObjectRef obj2 = owner;
    GrtNamedObjectRef obj3 = obj;

    // order from top level container to leaf, filling rest with NULL
    if (!obj1.is_valid())
      std::swap(obj1, obj2);
    if (!obj2.is_valid())
      std::swap(obj2, obj3);
    if (!obj1.is_valid())
      std::swap(obj1, obj2);

    log_db_obj_dropped(obj1, obj2, obj3);
    obj_list.remove_value(obj);
    return true;
  }
  return false;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_alter_table_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  const SqlAstNode *alter_list = tree->subitem(sql::_alter_commands, sql::_alter_list);
  if (!alter_list)
    return pr_irrelevant;

  // table + schema
  db_mysql_SchemaRef schema;
  db_mysql_TableRef obj;
  {
    std::string table_name = process_obj_full_name_item(tree->subitem(sql::_table_ident), &schema);
    obj = find_named_object_in_list(schema->tables(), table_name, _case_sensitive_identifiers);
  }
  if (!obj.is_valid())
    return pr_irrelevant;

  for (SqlAstNode::SubItemList::const_iterator it = alter_list->subitems()->begin(),
                                               it_end = alter_list->subitems()->end();
       it != it_end; ++it) {
    const SqlAstNode *alter_list_item = *it;
    if (alter_list_item->name_equals(sql::_alter_list_item)) {
      const SqlAstNode *item = (*it)->subitem(sql::_key_def);
      if (item) {
        if (item->find_subseq(sql::_FOREIGN, sql::_KEY_SYM))
          ENSURE(process_fk_item(item, obj), "Table `" + *obj->name() + "` : " + e.what())
        else if (item->subitem(sql::_key_list))
          ENSURE(process_index_item(item, obj), "Table `" + *obj->name() + "` : " + e.what())
      }
    }
  }

  return pr_processed;
}

Mysql_sql_parser::Parse_result Mysql_sql_parser::process_use_schema_statement(const SqlAstNode *tree) {
  // check if statement is relevant
  if (!tree->subseq(sql::_USE_SYM))
    return pr_irrelevant;

  const SqlAstNode *item = tree->subitem(sql::_ident);

  if (!item)
    throw Parse_exception("Invalid 'use' statement");

  set_active_schema(item->value());

  return pr_processed;
}

void Mysql_sql_parser::blame_existing_obj(bool critical, const GrtNamedObjectRef &obj,
                                          const GrtNamedObjectRef &container1, const GrtNamedObjectRef &container2) {
  if (_reuse_existing_objects)
    return;

  std::string err_text;
  err_text.append("Previously created ").append(obj.get_metaclass()->get_attribute("caption")).append(" `");
  if (container1.is_valid()) {
    err_text.append(*container1->name()).append("`.`");
  }
  if (container2.is_valid()) {
    err_text.append(*container2->name()).append("`.`");
  }
  err_text.append(*obj->name()).append("` was found. Statement ignored.");
  if (critical)
    throw Parse_exception(err_text);
  else
    add_log_message(err_text, 1);
}

template <typename T>
grt::Ref<T> Mysql_sql_parser::create_or_find_named_obj(const grt::ListRef<T> &obj_list, const std::string &obj_name,
                                                       bool case_sensitive, const GrtNamedObjectRef &container1,
                                                       const GrtNamedObjectRef &container2) {
  std::string time = base::fmttime(0, DATETIME_FMT);

  grt::Ref<T> obj;
  if (grt::Ref<T>::can_wrap(get_active_object())) {
    obj = grt::Ref<T>::cast_from(get_active_object());
    _reusing_existing_obj = true;
  } else {
    obj = find_named_object_in_list(obj_list, obj_name, case_sensitive);
    if (obj.is_valid()) {
      blame_existing_obj(true, obj, container1, container2);
      _reusing_existing_obj = true;
    } else {
      obj = grt::Ref<T>(grt::Initialized);
      obj->owner(container2.is_valid() ? container2
                                       : (container1.is_valid() ? container1 : GrtNamedObjectRef(_catalog)));
      try {
        obj.set_member("createDate", StringRef(time));
      } catch (std::exception &) {
      }
    }
  }

  try {
    obj.set_member("lastChangeDate", StringRef(time));
  } catch (std::exception &) {
  }

  return obj;
}

template <typename T>
grt::Ref<T> Mysql_sql_parser::create_or_find_named_routine(const grt::ListRef<T> &obj_list, const std::string &obj_name,
                                                           bool case_sensitive, const std::string &routine_type,
                                                           const GrtNamedObjectRef &container1,
                                                           const GrtNamedObjectRef &container2) {
  std::string time = base::fmttime(0, DATETIME_FMT);

  grt::Ref<T> obj;
  if (grt::Ref<T>::can_wrap(get_active_object())) {
    obj = grt::Ref<T>::cast_from(get_active_object());
    _reusing_existing_obj = true;
  } else {
    for (size_t c = obj_list.count(), i = 0; i < c; i++) {
      grt::Ref<T> item(obj_list[i]);
      if (*item->routineType() == routine_type) {
        if (base::string_compare(item->name(), obj_name, case_sensitive) == 0) {
          obj = item;
          break;
        }
      }
    }
    if (obj.is_valid()) {
      blame_existing_obj(true, obj, container1, container2);
      _reusing_existing_obj = true;
    } else {
      obj = grt::Ref<T>(grt::Initialized);
      obj->owner(container2.is_valid() ? container2
                                       : (container1.is_valid() ? container1 : GrtNamedObjectRef(_catalog)));
      try {
        obj.set_member("createDate", StringRef(time));
      } catch (std::exception &) {
      }
    }
  }

  try {
    obj.set_member("lastChangeDate", StringRef(time));
  } catch (std::exception &) {
  }

  return obj;
}
