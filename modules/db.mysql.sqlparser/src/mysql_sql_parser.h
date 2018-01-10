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

#include "mysql_sql_parser_base.h"
#include "grtsqlparser/sql_parser.h"
#include "grtsqlparser/fk_ref.h"

using namespace grt;

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC Mysql_sql_parser : protected Mysql_sql_parser_base, public Sql_parser {
public:
  typedef std::shared_ptr<Mysql_sql_parser> Ref;
  static Ref create() {
    return Ref(new Mysql_sql_parser());
  }
  virtual ~Mysql_sql_parser() {
  }

protected:
  Mysql_sql_parser();

public:
  virtual int parse_sql_script(db_CatalogRef catalog, const std::string &sql, grt::DictRef options);
  virtual int parse_sql_script_file(db_CatalogRef catalog, const std::string &filename, grt::DictRef options);

protected:
  typedef boost::function<Parse_result(const SqlAstNode *)> Process_specific_create_statement;

  grt::DictRef _datatype_cache;
  grt::StringRef _sql_script_codeset;
  grt::ListRef<GrtObject> _created_objects; // objects created during parsing (only if approp. option was set)
  bool _processing_create_statements;
  bool _processing_alter_statements;
  bool _processing_drop_statements;
  Process_specific_create_statement _process_specific_create_statement;
  Fk_ref_collection _fk_refs;
  bool _set_old_names;
  bool _reuse_existing_objects;            // allow to reuse objects created before parsing
  bool _reusing_existing_obj;              // if the object, currently being processed, existed before
  bool _stick_to_active_schema;            // some additional processing needed when things go out of defined scope
  db_mysql_TableRef _triggers_owner_table; // if specified, don't lookup owning table by name, but use specified one
  bool _gen_fk_names_when_empty;           // generate unique fk name when name is not given
  bool _strip_sql; // it's not wanted to strip input in editors, while it's so in most other cases
  Parse_result _last_parse_result;

  // higher level
  int parse_sql_script(db_CatalogRef &catalog, const std::string &sql, bool from_file, grt::DictRef &options);
  int process_sql_statement(const SqlAstNode *tree);
  Parse_result process_create_statement(const SqlAstNode *tree);
  Parse_result process_drop_statement(const SqlAstNode *tree);
  Parse_result process_alter_statement(const SqlAstNode *tree);
  void set_fk_references();

  // shapers
  typedef boost::function<void(db_mysql_SchemaRef &)> Shape_schema;
  Shape_schema _shape_schema;
  typedef boost::function<void(db_mysql_TableRef &)> Shape_table;
  Shape_table _shape_table;
  typedef boost::function<void(db_mysql_ViewRef &)> Shape_view;
  Shape_view _shape_view;
  typedef boost::function<void(db_mysql_RoutineRef &)> Shape_routine;
  Shape_routine _shape_routine;
  typedef boost::function<void(db_mysql_TriggerRef &)> Shape_trigger;
  Shape_trigger _shape_trigger;
  typedef boost::function<void(db_mysql_IndexRef &)> Shape_index;
  Shape_index _shape_index;
  typedef boost::function<void(db_mysql_LogFileGroupRef &)> Shape_logfile_group;
  Shape_logfile_group _shape_logfile_group;
  typedef boost::function<void(db_mysql_TablespaceRef &)> Shape_tablespace;
  Shape_tablespace _shape_tablespace;
  typedef boost::function<void(db_mysql_ServerLinkRef &)> Shape_serverlink;
  Shape_serverlink _shape_serverlink;
  void set_obj_name(GrtNamedObjectRef obj, const std::string &val);
  void set_obj_sql_def(db_DatabaseDdlObjectRef obj);

  // catalog helpers
  db_mysql_SchemaRef set_active_schema(const std::string &schema_name);
  db_mysql_SchemaRef ensure_schema_created(const std::string &schema_name, bool check_obj_name_uniqueness);
  void create_stub_table(db_mysql_SchemaRef &schema, db_mysql_TableRef &obj, const std::string &obj_name);
  void create_stub_column(db_mysql_TableRef &table, db_mysql_ColumnRef &obj, const std::string &obj_name,
                          db_mysql_ColumnRef tpl_obj);
  void blame_existing_obj(bool critical, const GrtNamedObjectRef &obj,
                          const GrtNamedObjectRef &container1 = GrtNamedObjectRef(),
                          const GrtNamedObjectRef &container2 = GrtNamedObjectRef());

  template <typename T>
  bool drop_obj(grt::ListRef<T> obj_list, const std::string &obj_name, bool if_exists,
                GrtNamedObjectRef owner = GrtNamedObjectRef(), GrtNamedObjectRef grand_owner = GrtNamedObjectRef());

  virtual GrtNamedObjectRef get_active_object() {
    return GrtNamedObjectRef();
  };

  template <typename T>
  grt::Ref<T> create_or_find_named_obj(const grt::ListRef<T> &obj_list, const std::string &obj_name,
                                       bool case_sensitive, const GrtNamedObjectRef &container1 = GrtNamedObjectRef(),
                                       const GrtNamedObjectRef &container2 = GrtNamedObjectRef());

  template <typename T>
  grt::Ref<T> create_or_find_named_routine(const grt::ListRef<T> &obj_list, const std::string &obj_name,
                                           bool case_sensitive, const std::string &routine_type,
                                           const GrtNamedObjectRef &container1 = GrtNamedObjectRef(),
                                           const GrtNamedObjectRef &container2 = GrtNamedObjectRef());

  // parse tree core
  Parse_result process_use_schema_statement(const SqlAstNode *tree);
  Parse_result process_create_schema_statement(const SqlAstNode *tree);
  Parse_result process_create_table_statement(const SqlAstNode *tree);
  Parse_result process_create_index_statement(const SqlAstNode *tree);
  Parse_result process_create_view_statement(const SqlAstNode *tree);
  Parse_result process_create_trigger_statement(const SqlAstNode *tree);
  Parse_result process_create_routine_statement(const SqlAstNode *tree);
  Parse_result process_create_server_link_statement(const SqlAstNode *tree);
  Parse_result process_create_tablespace_statement(const SqlAstNode *tree);
  Parse_result process_create_logfile_group_statement(const SqlAstNode *tree);

  Parse_result process_drop_schema_statement(const SqlAstNode *tree);
  Parse_result process_drop_table_statement(const SqlAstNode *tree);
  Parse_result process_drop_view_statement(const SqlAstNode *tree);
  Parse_result process_drop_routine_statement(const SqlAstNode *tree);
  Parse_result process_drop_trigger_statement(const SqlAstNode *tree);

  Parse_result process_alter_table_statement(const SqlAstNode *tree);

  // parse tree helpers
  std::string process_obj_full_name_item(const SqlAstNode *item, db_mysql_SchemaRef *schema);
  void process_field_type_item(const SqlAstNode *item, db_mysql_ColumnRef &column);
  void process_field_attributes_item(const SqlAstNode *item, db_mysql_ColumnRef &column, db_mysql_TableRef &table);
  std::string process_float_options_item(const SqlAstNode *item, std::string *precision = NULL,
                                         std::string *scale = NULL);
  std::string process_field_name_item(const SqlAstNode *item, GrtNamedObjectRef obj = GrtNamedObjectRef(),
                                      std::string *name3 = NULL, std::string *name2 = NULL, std::string *name1 = NULL);
  void process_index_item(const SqlAstNode *tree, db_mysql_TableRef &table);
  void process_fk_item(const SqlAstNode *tree, db_mysql_TableRef &table);
  void process_fk_references_item(const SqlAstNode *tree, db_mysql_ForeignKeyRef &fk, Fk_ref &fk_ref);
  void process_index_options_item(db_mysql_IndexRef &obj, const SqlAstNode *item);
  void process_index_kind_item(db_mysql_IndexRef &obj, const SqlAstNode *item);

  // prepare/clear routines
  void set_options(const grt::DictRef &options);
  void build_datatype_cache();
  void clear_datatype_cache();

  // logging
  void log_db_obj_created(const GrtNamedObjectRef &obj1, const GrtNamedObjectRef &obj2 = GrtNamedObjectRef(),
                          const GrtNamedObjectRef &obj3 = GrtNamedObjectRef());
  void log_db_obj_dropped(const GrtNamedObjectRef &obj1, const GrtNamedObjectRef &obj2 = GrtNamedObjectRef(),
                          const GrtNamedObjectRef &obj3 = GrtNamedObjectRef());
  void log_db_obj_operation(const std::string &op_name, const GrtNamedObjectRef &obj1,
                            const GrtNamedObjectRef &obj2 = GrtNamedObjectRef(),
                            const GrtNamedObjectRef &obj3 = GrtNamedObjectRef());

  // module utils
  void do_transactable_list_insert(grt::ListRef<GrtObject> list, GrtObjectRef object);

  class Active_schema_keeper {
  public:
    Active_schema_keeper(Mysql_sql_parser *sql_parser)
      : _sql_parser(sql_parser), _prev_schema(sql_parser->_active_schema) {
    }
    ~Active_schema_keeper() {
      _sql_parser->_active_schema = _prev_schema;
    }

  private:
    Mysql_sql_parser *_sql_parser;
    db_mysql_SchemaRef _prev_schema;
  };
  friend class Active_schema_keeper;

  class Null_state_keeper : public Mysql_sql_parser_base::Null_state_keeper {
  public:
    Null_state_keeper(Mysql_sql_parser *sql_parser)
      : Mysql_sql_parser_base::Null_state_keeper(sql_parser), _sql_parser(sql_parser) {
    }
    virtual ~Null_state_keeper();

  private:
    Mysql_sql_parser *_sql_parser;
  };
  friend class Null_state_keeper;
};
