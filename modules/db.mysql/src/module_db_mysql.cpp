/*
 * Copyright (c) 2009, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MSC_VER
#include <stdio.h>
#endif

#include <regex>
#include "base/sqlstring.h"

#include "grt/grt_manager.h"

#include "module_db_mysql.h"
#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "db_mysql_diffsqlgen.h"
#include "db_mysql_params.h"

#include "db_mysql_diffsqlgen_grant.h"
#include "db_mysql_catalog_report.h"
#include "base/string_utilities.h"
#include "base/sqlstring.h"
#include "base/util_functions.h"
#include "base/file_utilities.h"

#include "grtsqlparser/sql_specifics.h"
#include "sqlide/recordset_table_inserts_storage.h"
#include "sqlide/recordset_be.h"

#include "grts/structs.workbench.h"
#include "module_db_mysql_shared_code.h"
#include "grtdb/db_helpers.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace grt;
using namespace base;

static std::string get_table_old_name(db_mysql_TableRef table) {
  return std::string("`")
    .append(table->owner()->name().c_str())
    .append("`.`")
    .append(table->oldName().c_str())
    .append("` ");
}

inline std::string get_name(const GrtNamedObjectRef object, const bool use_short_names) {
  return use_short_names ? std::string("`").append(object->name().c_str()).append("`")
                         : get_qualified_schema_object_name(object);
};

DiffSQLGeneratorBEActionInterface::~DiffSQLGeneratorBEActionInterface() {
}

namespace {
  class TextPadding {
    int padding;
    int increment;

    std::string padding_text;

    void rebuild_padding_text() {
      padding_text = std::string(padding, ' ');
    }

  public:
    explicit TextPadding(int inc) : padding(0), increment(inc) {
    }

    TextPadding& operator++() {
      padding += increment;
      rebuild_padding_text();
      return *this;
    }

    TextPadding& operator--() {
      padding -= increment;
      rebuild_padding_text();
      return *this;
    }

    std::string& pad(std::string& text) {
      return text.append(padding_text);
    }
  };

  static std::string generate_single_partition(db_mysql_PartitionDefinitionRef part, bool is_range) {
    class Partition_options {
    public:
      static void generate(db_mysql_PartitionDefinitionRef part, std::string& sql) {
        if (strlen(part->comment().c_str()))
          sql.append(" COMMENT = '").append(base::escape_sql_string(part->comment().c_str())).append("'");

        if (strlen(part->dataDirectory().c_str()))
          sql.append(" DATA DIRECTORY = '").append(base::escape_sql_string(part->dataDirectory().c_str())).append("'");

        if (strlen(part->indexDirectory().c_str()))
          sql.append(" INDEX DIRECTORY = '")
            .append(base::escape_sql_string(part->indexDirectory().c_str()))
            .append("'");

        if (strlen(part->maxRows().c_str()))
          sql.append(" MAX_ROWS = ").append(base::escape_sql_string(part->maxRows().c_str()));

        if (strlen(part->minRows().c_str()))
          sql.append(" MIN_ROWS = ").append(base::escape_sql_string(part->minRows().c_str()));

        // TODO: process TABLESPACE and NODEGROUP cluster-specific options
      }
    };

    std::string sql;

    sql.append(" PARTITION ");

    sql.append(part->name().c_str()).append(" VALUES ");

    if (is_range)
      sql.append("LESS THAN (").append(part->value().c_str()).append(")");
    else
      sql.append("IN (").append(part->value().c_str()).append(")");

    Partition_options::generate(part, sql);

    if (part->subpartitionDefinitions().count() > 0) {
      sql.append(" (");
      for (size_t subsz = part->subpartitionDefinitions().count(), j = 0; j < subsz; j++) {
        if (j > 0)
          sql.append(",");

        db_mysql_PartitionDefinitionRef subpart = part->subpartitionDefinitions().get(j);
        sql.append(" SUBPARTITION ").append(subpart->name().c_str());

        Partition_options::generate(subpart, sql);
      }
      sql.append(")");
    }

    return sql;
  }

  static std::string generate_drop_partitions(const std::list<std::string>& part_names) {
    std::string sql(" DROP PARTITION ");
    bool first = true;

    for (std::list<std::string>::const_iterator e = part_names.end(), it = part_names.begin(); it != e; it++) {
      if (first)
        first = false;
      else
        sql.append(", ");

      sql.append(*it);
    }

    return sql;
  }

  static std::string global_generate_create(db_mysql_ForeignKeyRef fk, TextPadding& padding, bool use_short_names) {
    std::string sql;

    sql.append("CONSTRAINT `").append(fk->name().c_str()).append("`\n");

    ++padding;
    padding.pad(sql).append("FOREIGN KEY (");

    grt::ListRef<db_Column> fk_columns = fk->columns();
    for (size_t fk_column_count = fk_columns.count(), j = 0; j < fk_column_count; j++) {
      if (j != 0)
        sql.append(", ");

      db_ColumnRef fk_column = fk_columns.get(j);

      sql.append("`").append(fk_column->name().c_str()).append("` ");
    }
    sql = base::trim_right(sql);

    sql.append(")\n");
    padding.pad(sql).append("REFERENCES `");
    if (fk->referencedTable().is_valid()) {
      // Omit schema name if we use short names and the referenced table is in the same schema as the
      // referencing table.
      if (use_short_names && (fk->owner()->owner() == fk->referencedTable()->owner()))
        sql.append(fk->referencedTable()->name().c_str());
      else
        sql.append(fk->referencedTable()->owner()->name().c_str())
          .append("`.`")
          .append(fk->referencedTable()->name().c_str());
    }
    sql.append("` (");

    grt::ListRef<db_Column> ref_fk_columns = fk->referencedColumns();
    for (size_t ref_fk_column_count = ref_fk_columns.count(), j = 0; j < ref_fk_column_count; j++) {
      if (j > 0)
        sql.append(", ");

      db_ColumnRef ref_fk_column = ref_fk_columns.get(j);
      if (!ref_fk_column.is_valid())
        continue;
      sql.append("`").append(ref_fk_column->name().c_str()).append("` ");
    }
    sql = base::trim_right(sql);
    sql.append(")");

    if (strlen(fk->deleteRule().c_str())) {
      sql.append("\n");
      padding.pad(sql).append("ON DELETE ").append(fk->deleteRule().c_str());
    }

    if (strlen(fk->updateRule().c_str())) {
      sql.append("\n");
      padding.pad(sql).append("ON UPDATE ").append(fk->updateRule().c_str());
    }

    --padding;
    return sql;
  }

  static std::string generate_drop_index(db_mysql_IndexRef index) {
    /*
     | DROP {INDEX|KEY} index_name
     */
    if (index->isPrimary())
      return std::string("DROP PRIMARY KEY");

    std::string index_name;
    {
      if (index->oldName().empty())
        index_name = " ";
      else
        index_name = strfmt("`%s` ", index->oldName().c_str());
    }
    return strfmt("DROP INDEX %s", index_name.c_str());
  }

  class ActionGenerateSQL : public DiffSQLGeneratorBEActionInterface {
    TextPadding padding;

    bool _case_sensitive;
    bool _use_oids_as_dict_key;
    int _maxTableCommentLength;
    int _maxIndexCommentLength;
    int _maxColumnCommentLength;
    std::string _algorithm_type;
    std::string _lock_type;

    std::string sql;
    std::string indexAlter;
    std::string comma;
    std::string table_q_name;
    size_t empty_length;
    bool first_column, first_change, first_fk_create, first_fk_drop;
    std::string _non_std_sql_delimiter;

    std::string fk_add_sql;
    std::string fk_drop_sql;

    std::list<std::string> partitions_to_drop;
    std::list<std::string> partitions_to_change;
    std::list<std::string> partitions_to_add;

    grt::DictRef target_map;
    grt::StringListRef target_list;
    grt::ListRef<GrtNamedObject> target_object_list;
    bool disable_object_list;

    void remember_alter(const GrtNamedObjectRef& obj, const std::string& sql);
    void remember(const GrtNamedObjectRef& obj, const std::string& sql, const bool front = false);

    void alter_table_property(std::string& to, const std::string& name, const std::string& value);

  public:
    ActionGenerateSQL(grt::ValueRef target, grt::ListRef<GrtNamedObject> obj_list, const grt::DictRef options,
                      bool use_oids_as_key);
    virtual ~ActionGenerateSQL();

    // create table
    void create_table_props_begin(db_mysql_TableRef);
    void create_table_props_end(db_mysql_TableRef);

    void create_table_columns_begin(db_mysql_TableRef);
    void create_table_column(db_mysql_ColumnRef);
    void create_table_columns_end(db_mysql_TableRef);
    std::string generate_create(db_mysql_ColumnRef column);

    void create_table_indexes_begin(db_mysql_TableRef);
    void create_table_index(db_mysql_IndexRef, bool gen_create_index);
    void create_table_indexes_end(db_mysql_TableRef);
    std::string generate_create(db_mysql_IndexRef index, std::string table_q_name, bool separate_index);

    void create_table_fks_begin(db_mysql_TableRef);
    void create_table_fk(db_mysql_ForeignKeyRef);
    void create_table_fks_end(db_mysql_TableRef);

    void create_table_engine(grt::StringRef);
    void create_table_next_auto_inc(grt::StringRef);
    void create_table_password(grt::StringRef);
    void create_table_delay_key_write(grt::IntegerRef);
    void create_table_charset(grt::StringRef);
    void create_table_collate(grt::StringRef);
    void create_table_merge_union(grt::StringRef);
    void create_table_merge_insert(grt::StringRef);
    void create_table_pack_keys(grt::StringRef);
    void create_table_checksum(grt::IntegerRef);
    void create_table_row_format(grt::StringRef);
    void create_table_key_block_size(grt::StringRef);
    void create_table_avg_row_length(grt::StringRef);
    void create_table_min_rows(grt::StringRef);
    void create_table_max_rows(grt::StringRef);
    void create_table_comment(grt::StringRef);
    void create_table_data_dir(grt::StringRef);
    void create_table_index_dir(grt::StringRef);

    // drop table
    void drop_table(db_mysql_TableRef);

    // schema drop/create
    void create_schema(db_mysql_SchemaRef);
    void drop_schema(db_mysql_SchemaRef);

    // alter schema
    void alter_schema_props_begin(db_mysql_SchemaRef);
    void alter_schema_default_charset(db_mysql_SchemaRef, grt::StringRef value);
    void alter_schema_default_collate(db_mysql_SchemaRef, grt::StringRef value);
    void alter_schema_name(db_mysql_SchemaRef, grt::StringRef value);
    void alter_schema_props_end(db_mysql_SchemaRef);

    void alter_table_props_begin(db_mysql_TableRef);
    void alter_table_name(db_mysql_TableRef, grt::StringRef);
    void alter_table_engine(db_mysql_TableRef, grt::StringRef);
    void alter_table_next_auto_inc(db_mysql_TableRef, grt::StringRef);
    void alter_table_password(db_mysql_TableRef, grt::StringRef);
    void alter_table_delay_key_write(db_mysql_TableRef, grt::IntegerRef);
    void alter_table_charset(db_mysql_TableRef, grt::StringRef);
    void alter_table_collate(db_mysql_TableRef, grt::StringRef);
    void alter_table_comment(db_mysql_TableRef, grt::StringRef);
    void alter_table_merge_union(db_mysql_TableRef, grt::StringRef);
    void alter_table_merge_insert(db_mysql_TableRef, grt::StringRef);
    void alter_table_pack_keys(db_mysql_TableRef, grt::StringRef);
    void alter_table_checksum(db_mysql_TableRef, grt::IntegerRef);
    void alter_table_row_format(db_mysql_TableRef, grt::StringRef);
    void alter_table_key_block_size(db_mysql_TableRef, grt::StringRef);
    void alter_table_avg_row_length(db_mysql_TableRef, grt::StringRef);
    void alter_table_min_rows(db_mysql_TableRef, grt::StringRef);
    void alter_table_max_rows(db_mysql_TableRef, grt::StringRef);
    void alter_table_connection_string(db_mysql_TableRef, grt::StringRef);
    void alter_table_generate_partitioning(db_mysql_TableRef table, const std::string& part_type,
                                           const std::string& part_expr, int part_count,
                                           const std::string& subpart_type, const std::string& subpart_expr,
                                           grt::ListRef<db_mysql_PartitionDefinition> part_defs);
    void alter_table_drop_partitioning(db_mysql_TableRef table);
    void alter_table_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range);
    void alter_table_reorganize_partition(db_mysql_PartitionDefinitionRef old_part,
                                          db_mysql_PartitionDefinitionRef new_part, bool is_range);
    void alter_table_drop_partition(const std::string& part_name);
    void alter_table_partition_count(db_mysql_TableRef, grt::IntegerRef);
    void alter_table_partition_definitions(db_mysql_TableRef, grt::StringRef);
    void alter_table_props_end(db_mysql_TableRef);

    void alter_table_columns_begin(db_mysql_TableRef);
    void alter_table_add_column(db_mysql_TableRef, std::map<std::string, std::string>, db_mysql_ColumnRef,
                                db_mysql_ColumnRef after);
    void alter_table_drop_column(db_mysql_TableRef, db_mysql_ColumnRef);
    void alter_table_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col, db_mysql_ColumnRef mod_col,
                                   db_mysql_ColumnRef after, bool modified,
                                   std::map<std::string, std::string> column_rename_map);
    void alter_table_columns_end(db_mysql_TableRef);

    void alter_table_indexes_begin(db_mysql_TableRef);
    void alter_table_add_index(db_mysql_IndexRef);
    void alter_table_drop_index(db_mysql_IndexRef);
    void alter_table_indexes_end(db_mysql_TableRef);
    void alter_table_change_index(db_mysql_IndexRef orgIndex, db_mysql_IndexRef newIndex);

    void alter_table_fks_begin(db_mysql_TableRef);
    void alter_table_add_fk(db_mysql_ForeignKeyRef);
    void alter_table_drop_fk(db_mysql_ForeignKeyRef);
    void alter_table_fks_end(db_mysql_TableRef);

    // triggers
    void create_trigger(db_mysql_TriggerRef, bool for_alter);
    void drop_trigger(db_mysql_TriggerRef, bool for_alter);

    // views
    void create_view(db_mysql_ViewRef);
    void drop_view(db_mysql_ViewRef);

    // routines
    void create_routine(db_mysql_RoutineRef, bool for_alter);
    void drop_routine(db_mysql_RoutineRef, bool for_alter);

    // users
    void create_user(db_UserRef);
    void drop_user(db_UserRef);

    std::string get_name(GrtNamedObjectRef object) const {
      return ::get_name(object, _omitSchemas);
    };
    std::string generate_add_index(db_mysql_IndexRef index);

    virtual void disable_list_insert(const bool flag) {
      disable_object_list = flag;
    };
  };

  ActionGenerateSQL::ActionGenerateSQL(grt::ValueRef target, grt::ListRef<GrtNamedObject> obj_list,
                                       const grt::DictRef options, bool use_oids_as_key = false)
    : padding(2), _use_oids_as_dict_key(use_oids_as_key), disable_object_list(false) {
    first_column = false;
    first_change = false;
    empty_length = 0;
    first_fk_create = false;
    first_fk_drop = false;
    _case_sensitive = options.get_int("CaseSensitive") != 0;
    _maxTableCommentLength = (int)options.get_int("maxTableCommentLength");
    _maxIndexCommentLength = (int)options.get_int("maxIndexCommentLength");
    _maxColumnCommentLength = (int)options.get_int("maxColumnCommentLength");
    _algorithm_type = options.get_string("AlterAlgorithm");
    _lock_type = options.get_string("AlterLock");

    _use_oids_as_dict_key = options.get_int("UseOIDAsResultDictKey", use_oids_as_key) != 0;

    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql");
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
    _non_std_sql_delimiter = bec::GRTManager::get()->get_app_option_string("SqlDelimiter", "$$");

    if (target.type() == DictType) {
      this->target_list = grt::StringListRef();
      this->target_map = grt::DictRef::cast_from(target);
    } else if (target.type() == ListType) {
      this->target_list = grt::StringListRef::cast_from(target);
      this->target_map = grt::DictRef();
    }
    this->target_object_list = obj_list;
  }

  ActionGenerateSQL::~ActionGenerateSQL() {
  }

  // create table methods

  void ActionGenerateSQL::create_table_props_begin(db_mysql_TableRef table) {
    sql.assign("CREATE");

    table_q_name = get_name(table);

    if (table->isTemporary())
      sql.append(" TEMPORARY");

    sql.append(" TABLE");
    if (_put_if_exists)
      sql.append(" IF NOT EXISTS");
    sql.append(" ").append(table_q_name).append(" (\n");
    ++padding;
  }

  void ActionGenerateSQL::create_table_props_end(db_mysql_TableRef table) {
    remember(table, sql);
  }

  void ActionGenerateSQL::create_table_columns_begin(db_mysql_TableRef) {
    first_column = true;
  }

  void ActionGenerateSQL::create_table_column(db_mysql_ColumnRef column) {
    if (first_column)
      first_column = false;
    else
      sql.append(",\n");

    padding.pad(sql).append(generate_create(column));
  }

  void ActionGenerateSQL::create_table_columns_end(db_mysql_TableRef) {
  }

  std::string ActionGenerateSQL::generate_create(db_mysql_ColumnRef column) {
    std::string sql;

    sql.append("`").append(column->name().c_str()).append("` ");

    sql.append(column->formattedType());

    sql.append(" ");

    if (column->simpleType().is_valid())
      if (*column->simpleType()->name() != "JSON" &&
          (!column->simpleType()->group().is_valid() || *column->simpleType()->group()->name() == "string" ||
           *column->simpleType()->group()->name() == "text" || *column->simpleType()->name() == "ENUM")) {
        if (!(*column->characterSetName()).empty())
          sql.append("CHARACTER SET '").append(column->characterSetName()).append("' ");
        if (!(*column->collationName()).empty() &&
            (charsetForCollation(column->collationName()) == (*column->characterSetName())))
          sql.append("COLLATE '").append(column->collationName()).append("' ");
      }

    if (column->generated()) {
      sql += "GENERATED ALWAYS AS (" + *column->expression() + ") " + *column->generatedStorage() + " ";
    } else {
      if (column->simpleType().is_valid()) {
        grt::StringListRef flags = column->flags();
        size_t flags_count = flags.count();
        for (size_t j = 0; j < flags_count; j++)
          sql.append(flags.get(j).c_str()).append(" ");
      } else if (column->userType().is_valid() && !column->userType()->flags().empty())
        sql.append(column->userType()->flags()).append(" ");

      if (column->isNotNull())
        sql.append("NOT NULL ");
      else
        sql.append("NULL ");

      if (column->defaultValueIsNull())
        sql.append("DEFAULT NULL ");
      else if (column->defaultValue().is_valid() && column->defaultValue().c_str() &&
               (strlen(column->defaultValue().c_str()) > 0)) {
        std::string default_value = toupper(column->defaultValue());
        if (!((column->simpleType().is_valid()) && (column->simpleType()->name() == "TIMESTAMP") &&
              (default_value.find("ON UPDATE") == 0)))
          sql.append("DEFAULT ");
        sql.append(column->defaultValue().c_str()).append(" ");
      }
    }

    if (column->autoIncrement()) {
      db_SimpleDatatypeRef columnType;

      // Determine actually used column type first.
      if (column->userType().is_valid() && column->userType()->actualType().is_valid())
        columnType = column->userType()->actualType();
      else if (column->simpleType().is_valid() && column->simpleType()->group().is_valid())
        columnType = column->simpleType();

      if (columnType.is_valid() && columnType->group().is_valid() &&
          !strcmp(columnType->group()->name().c_str(), "numeric"))
        sql.append("AUTO_INCREMENT ");
    }

    std::string comment = bec::TableHelper::generate_comment_text(column->comment(), _maxColumnCommentLength);
    if (!comment.empty())
      sql.append("COMMENT ").append(comment).append(" ");

    // TODO:
    // (?) [reference_definition]
    // [COLUMN_FORMAT {FIXED|DYNAMIC|DEFAULT}]
    // [STORAGE {DISK|MEMORY}]

    return base::trim_right(sql);
  }

  void ActionGenerateSQL::create_table_indexes_begin(db_mysql_TableRef) {
  }

  void ActionGenerateSQL::create_table_index(db_mysql_IndexRef index, bool gen_create_index) {
    std::string index_sql(generate_create(index, table_q_name, gen_create_index));

    if (gen_create_index) {
      index_sql = std::string("CREATE ").append(index_sql);
      remember(index, index_sql);
    } else {
      sql.append(",\n");
      padding.pad(sql).append(index_sql);
    }
  }

  void ActionGenerateSQL::create_table_indexes_end(db_mysql_TableRef) {
  }

  //--------------------------------------------------------------------------------------------------------------------

  std::string ActionGenerateSQL::generate_create(db_mysql_IndexRef index, std::string table_q_name,
                                                 bool separate_index) {
    std::stringstream result;
    bool pk = (index->isPrimary() != 0);

    separate_index = (!pk && separate_index); // pk cannot be added via CREATE INDEX
    bool isFullTextSpatialOrHashIndex = false;

    std::string indexType = base::toupper(index->indexType());
    if (pk) {
      result << "PRIMARY KEY";
    } else if (index->unique() != 0) {
      result << "UNIQUE INDEX";
    } else if (!indexType.empty()) {
      if (indexType == "SPATIAL" || indexType == "FULLTEXT")
        isFullTextSpatialOrHashIndex = true;
      result << indexType;

      if (!base::hasSuffix(indexType, "KEY") && !base::hasSuffix(indexType, "INDEX"))
        result << " INDEX";
    } else {
      result << " INDEX";
    }

    if (!pk && !index->name().empty())
      result << " `" << *index->name() << "`";

    if (!index->indexKind().empty()) {
      isFullTextSpatialOrHashIndex = true;
      result << " USING " << *index->indexKind();
    }

    if (separate_index)
      result << " ON " << table_q_name;

    result << " (";

    grt::ListRef<db_mysql_IndexColumn> ind_columns = index->columns();
    for (size_t index_column_count = ind_columns.count(), j = 0; j < index_column_count; j++) {
      db_IndexColumnRef ind_column = ind_columns.get(j);

      if (j > 0)
        result << ", ";

      db_ColumnRef col = ind_column->referencedColumn();
      if (col.is_valid())
        result << "`" << *col->name() << "`";

      if (ind_column->columnLength() > 0)
        result << "(" << ind_column->columnLength() << ")";

      if (!pk && index->indexKind().empty() && !isFullTextSpatialOrHashIndex)
        result << " " << ((ind_column->descend() == 0 ? "ASC" : "DESC"));
    }
    result << ")";

    if (index->keyBlockSize())
      result << " KEY_BLOCK_SIZE = " << index->keyBlockSize();

    if (index->withParser().is_valid() && *index->withParser().c_str())
      result << " WITH PARSER " << *index->withParser();

    std::string comment = bec::TableHelper::generate_comment_text(index->comment(), _maxIndexCommentLength);
    if (!comment.empty())
      result << " COMMENT " << comment;

    auto catalog = db_CatalogRef::cast_from(index->owner()->owner()->owner());

    GrtVersionRef version;
    if (catalog->owner().is_valid())
      version = GrtVersionRef::cast_from(
        bec::getModelOption(workbench_physical_ModelRef::cast_from(catalog->owner()), "CatalogVersion"));
    else
      version = catalog->version();

    if (bec::is_supported_mysql_version_at_least(version, 8, 0, 0)) {
      auto table = db_mysql_TableRef::cast_from(index->owner());

      if (index->isPrimary() == 0 && (index->unique() == 0 || table->indices().count() > 1)) {
        if (index->visible() == 1)
          result << " VISIBLE";
        else
          result << " INVISIBLE";
      }
    }

    return result.str();
  }

  //--------------------------------------------------------------------------------------------------------------------

  void ActionGenerateSQL::create_table_fks_begin(db_mysql_TableRef) {
  }

  //--------------------------------------------------------------------------------------------------------------------

  void ActionGenerateSQL::create_table_fk(db_mysql_ForeignKeyRef fk) {
    grt::StringRef ename = db_mysql_TableRef::cast_from(fk->owner())->tableEngine();
    db_mysql_StorageEngineRef engine = bec::TableHelper::get_engine_by_name(ename);
    if (engine.is_valid() && !engine->supportsForeignKeys())
      return;

    sql.append(",\n");
    padding.pad(sql).append(global_generate_create(fk, padding, _omitSchemas));
  }

  void ActionGenerateSQL::create_table_fks_end(db_mysql_TableRef) {
    sql.append(")");
    --padding;
  }

  void ActionGenerateSQL::create_table_engine(grt::StringRef value) {
    sql.append("\nENGINE = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_next_auto_inc(grt::StringRef value) {
    sql.append("\nAUTO_INCREMENT = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_password(grt::StringRef value) {
    sql.append("\nPASSWORD = '").append(value.c_str()).append("'");
  }

  void ActionGenerateSQL::create_table_delay_key_write(grt::IntegerRef value) {
    sql.append("\nDELAY_KEY_WRITE = ").append(value.toString());
  }

  void ActionGenerateSQL::create_table_charset(grt::StringRef value) {
    sql.append("\nDEFAULT CHARACTER SET = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_collate(grt::StringRef value) {
    sql.append("\nCOLLATE = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_merge_union(grt::StringRef value) {
    std::string s = *value;
    if (!s.empty() && s[0] == '(')
      sql.append("\nUNION = ").append(value.c_str());
    else
      sql.append("\nUNION = (").append(s).append(")");
  }

  void ActionGenerateSQL::create_table_merge_insert(grt::StringRef value) {
    sql.append("\nINSERT_METHOD = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_pack_keys(grt::StringRef value) {
    sql.append("\nPACK_KEYS = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_checksum(grt::IntegerRef value) {
    sql.append("\nCHECKSUM = ").append(value.toString());
  }

  void ActionGenerateSQL::create_table_row_format(grt::StringRef value) {
    sql.append("\nROW_FORMAT = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_key_block_size(grt::StringRef value) {
    sql.append("\nKEY_BLOCK_SIZE = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_avg_row_length(grt::StringRef value) {
    sql.append("\nAVG_ROW_LENGTH = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_min_rows(grt::StringRef value) {
    sql.append("\nMIN_ROWS = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_max_rows(grt::StringRef value) {
    sql.append("\nMAX_ROWS = ").append(value.c_str());
  }

  void ActionGenerateSQL::create_table_comment(grt::StringRef value) {
    sql.append("\nCOMMENT = ").append(bec::TableHelper::generate_comment_text(value, _maxTableCommentLength));
  }

  void ActionGenerateSQL::create_table_data_dir(grt::StringRef value) {
    sql.append("\nDATA DIRECTORY = '").append(value.c_str()).append("'");
  }

  void ActionGenerateSQL::create_table_index_dir(grt::StringRef value) {
    sql.append("\nINDEX DIRECTORY = '").append(value.c_str()).append("'");
  }

  // drop table
  void ActionGenerateSQL::drop_table(db_mysql_TableRef table) {
    sql.clear();
    sql.append("DROP TABLE IF EXISTS ").append(get_name(table)).append(" ");
    remember(table, sql);
  }

  // schema

  void ActionGenerateSQL::create_schema(db_mysql_SchemaRef schema) {
    std::string schema_sql;
    schema_sql.append("CREATE SCHEMA ");
    if (_put_if_exists)
      schema_sql.append("IF NOT EXISTS ");
    schema_sql.append("`").append(schema->name().c_str()).append("` ");

    if (schema->defaultCharacterSetName().is_valid()) {
      std::string charset = schema->defaultCharacterSetName();
      if (!charset.empty()) {
        schema_sql += "DEFAULT CHARACTER SET " + charset + " ";
        if (schema->defaultCollationName().is_valid()) {
          std::string collation = schema->defaultCollationName();
          if (!collation.empty() && (charsetForCollation(collation) == charset) &&
              (defaultCollationForCharset(charset) != collation))
            schema_sql += "COLLATE " + collation + " ";
        }
      }
    }

    remember(schema, schema_sql);
  }

  void ActionGenerateSQL::drop_schema(db_mysql_SchemaRef schema) {
    std::string schema_sql;
    schema_sql.append("DROP SCHEMA IF EXISTS `").append(schema->name().c_str()).append("` ");
    remember(schema, schema_sql, true);
  }

  // alter schema methods

  void ActionGenerateSQL::alter_schema_props_begin(db_mysql_SchemaRef schema) {
    sql.clear();
  }

  void ActionGenerateSQL::alter_schema_name(db_mysql_SchemaRef schema, grt::StringRef value) {
    std::string rename_sql("RENAME SCHEMA `");
    rename_sql += schema->name().c_str();
    rename_sql += "` TO `";
    rename_sql += value.c_str();
    rename_sql += "`";
    remember_alter(schema, rename_sql);
  }

  void ActionGenerateSQL::alter_schema_default_charset(db_mysql_SchemaRef schema, grt::StringRef value) {
    sql.append(" DEFAULT CHARACTER SET ").append(value).append(" ");
  }

  void ActionGenerateSQL::alter_schema_default_collate(db_mysql_SchemaRef schema, grt::StringRef value) {
    if (value.empty())
      sql.append(" DEFAULT COLLATE ")
        .append(bec::get_default_collation_for_charset(db_SchemaRef::cast_from(schema),
                                                       schema->defaultCharacterSetName().c_str()))
        .append(" ");
    else
      sql.append(" DEFAULT COLLATE ").append(value).append(" ");
  }

  void ActionGenerateSQL::alter_schema_props_end(db_mysql_SchemaRef schema) {
    if (!sql.empty()) {
      sql = std::string("ALTER SCHEMA `").append(schema->name().c_str()).append("` ").append(sql);
      remember_alter(schema, sql);
    }
  }

  // alter table

  void ActionGenerateSQL::alter_table_props_begin(db_mysql_TableRef table) {
    comma.clear();
    sql.assign("ALTER TABLE ");
    sql += get_table_old_name(table) + "\n";
    empty_length = sql.length();

    partitions_to_drop.clear();
    partitions_to_change.clear();
    partitions_to_add.clear();
    first_change = true;
  }

  void ActionGenerateSQL::alter_table_property(std::string& to, const std::string& name, const std::string& value) {
    if (first_change)
      first_change = false;
    else
      to.append(", ");

    to.append(name).append(value).append(" ");
  }

  void ActionGenerateSQL::alter_table_name(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(
      sql, "RENAME TO ",
      _omitSchemas
        ? std::string(" `").append(str.c_str()).append("`")
        : std::string(" `").append(table->owner()->name().c_str()).append("`.`").append(str.c_str()).append("`"));
  }

  void ActionGenerateSQL::alter_table_engine(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "ENGINE = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_next_auto_inc(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "AUTO_INCREMENT = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_password(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "PASSWORD  = '", std::string(str.c_str()).append("' "));
  }

  void ActionGenerateSQL::alter_table_delay_key_write(db_mysql_TableRef table, grt::IntegerRef n) {
    alter_table_property(sql, "DELAY_KEY_WRITE  = ", n.toString());
  }

  void ActionGenerateSQL::alter_table_charset(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "CHARACTER SET = ", str.empty() ? "DEFAULT" : str.c_str());
  }

  void ActionGenerateSQL::alter_table_collate(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "COLLATE = ", str.empty() ? "DEFAULT" : str.c_str());
  }

  void ActionGenerateSQL::alter_table_comment(db_mysql_TableRef table, grt::StringRef str) {
    std::string comment = bec::TableHelper::generate_comment_text(str, _maxTableCommentLength);
    if (comment.empty())
      alter_table_property(sql, "COMMENT = ", "''");
    else
      alter_table_property(sql, "COMMENT = ", comment);
  }

  void ActionGenerateSQL::alter_table_merge_union(db_mysql_TableRef table, grt::StringRef str) {
    std::string s = *str;
    if (!s.empty() && s[0] == '(')
      s = s.substr(1, s.size() - 2);

    if (!_omitSchemas)
      s = bec::TableHelper::normalize_table_name_list(table->owner()->name(), s);

    alter_table_property(sql, "UNION = (", std::string(s).append(") "));
  }

  void ActionGenerateSQL::alter_table_merge_insert(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "INSERT_METHOD = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_pack_keys(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "PACK_KEYS = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_checksum(db_mysql_TableRef table, grt::IntegerRef n) {
    alter_table_property(sql, "CHECKSUM = ", n.toString());
  }

  void ActionGenerateSQL::alter_table_row_format(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "ROW_FORMAT = ", str.empty() ? "DEFAULT" : str.c_str());
  }

  void ActionGenerateSQL::alter_table_key_block_size(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "KEY_BLOCK_SIZE = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_avg_row_length(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "AVG_ROW_LENGTH = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_min_rows(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "MIN_ROWS = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_max_rows(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "MAX_ROWS = ", str.c_str());
  }

  void ActionGenerateSQL::alter_table_connection_string(db_mysql_TableRef table, grt::StringRef str) {
    alter_table_property(sql, "CONNECTION = ", str.c_str());
  }

  // used by generate_create_partitioning and generate_add_partitioning
  // generates full PARTITION BY clause
  void ActionGenerateSQL::alter_table_generate_partitioning(db_mysql_TableRef table, const std::string& part_type,
                                                            const std::string& part_expr, int part_count,
                                                            const std::string& subpart_type,
                                                            const std::string& subpart_expr,
                                                            grt::ListRef<db_mysql_PartitionDefinition> part_defs) {
    if (!part_count) {
      alter_table_drop_partitioning(table);
      return;
    }
    bool is_range = (part_type.compare("RANGE") == 0);
    bool is_list = false;

    if (!is_range)
      is_list = (part_type.compare("LIST") == 0);

    std::string partition_sql(" PARTITION BY ");

    std::stringstream ss;
    ss << part_count;
    partition_sql.append(part_type).append("(").append(part_expr).append(") PARTITIONS ").append(ss.str());

    if (is_range || is_list) {
      if (!subpart_type.empty()) {
        partition_sql.append(" SUBPARTITION BY ").append(subpart_type).append("(").append(subpart_expr).append(") ");
      }

      partition_sql.append("(");

      for (size_t sz = part_defs.count(), i = 0; i < sz; i++) {
        if (i > 0)
          partition_sql.append(", ");

        db_mysql_PartitionDefinitionRef part = part_defs.get(i);
        partition_sql.append(generate_single_partition(part, is_range));
      }
      partition_sql.append(")");
    }

    sql.append(comma).append(partition_sql);
    sql.append(" ");
  }

  void ActionGenerateSQL::alter_table_drop_partitioning(db_mysql_TableRef table) {
    sql.append(" REMOVE PARTITIONING ");
  }

  void ActionGenerateSQL::alter_table_reorganize_partition(db_mysql_PartitionDefinitionRef old_part,
                                                           db_mysql_PartitionDefinitionRef new_part, bool is_range) {
    std::string part_sql(" REORGANIZE PARTITION ");

    part_sql.append(old_part->name().c_str())
      .append(" INTO (")
      .append(generate_single_partition(new_part, is_range))
      .append(")");

    partitions_to_change.push_back(part_sql);
  }

  void ActionGenerateSQL::alter_table_drop_partition(const std::string& part_name) {
    partitions_to_drop.push_back(part_name.c_str());
  }

  void ActionGenerateSQL::alter_table_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range) {
    partitions_to_add.push_back(
      std::string(" ADD PARTITION (").append(generate_single_partition(part, is_range)).append(") "));
  }

  void ActionGenerateSQL::alter_table_partition_count(db_mysql_TableRef table, grt::IntegerRef oldcount) {
    // we get here only if partitionType was not changed, so we can rely on old type setting

    ssize_t newcount = table->partitionCount();
    std::string part_type(table->partitionType().c_str());

    // for range/list partitions we use add/drop/reorganize partitions
    if ((oldcount == newcount) ||
        ((part_type.find("HASH") == std::string::npos) && (part_type.find("KEY") == std::string::npos)))
      return;

    std::string part_count_sql;

    if (oldcount > newcount) // merge
      part_count_sql.append(" COALESCE PARTITION ").append(IntegerRef(oldcount - newcount).toString());
    else // split
      part_count_sql.append(" ADD PARTITION PARTITIONS ").append(IntegerRef(newcount - oldcount).toString());

    // partition count alone can be changed only for HASH/KEY partitions
    // generate_change_partition_count() will return empty string otherwise
    // for RANGE/LIST we ignore change of this attribute and rely solely on
    // partition definitions change
    if (!part_count_sql.empty()) {
      sql.append(comma).append(part_count_sql);
      // partitions_processed= true;
    }
  }

  void ActionGenerateSQL::alter_table_partition_definitions(db_mysql_TableRef table, grt::StringRef str) {
  }

  void ActionGenerateSQL::alter_table_props_end(db_mysql_TableRef table) {
    if (sql.length() > empty_length) {
      if (!_algorithm_type.empty())
        sql.append(", ALGORITHM = ").append(_algorithm_type);

      if (!_lock_type.empty())
        sql.append(", LOCK = ").append(_lock_type);
    }

    if (!partitions_to_drop.empty()) {
      sql.append(generate_drop_partitions(partitions_to_drop));

      remember_alter(table, sql);
      sql.assign("ALTER TABLE ");
      sql.append(get_table_old_name(table));
    }

    if (!partitions_to_change.empty()) {
      for (std::list<std::string>::const_iterator e = partitions_to_change.end(), it = partitions_to_change.begin();
           it != e; it++) {
        sql.append(*it);
        remember_alter(table, sql);
        sql.assign("ALTER TABLE ");
        sql.append(get_table_old_name(table));
      }
    }

    if (!partitions_to_add.empty()) {
      for (std::list<std::string>::const_iterator e = partitions_to_add.end(), it = partitions_to_add.begin(); it != e;
           it++) {
        sql.append(*it);
        remember_alter(table, sql);
        sql.assign("ALTER TABLE ");
        sql.append(get_table_old_name(table));
      }
    }

    if (sql.length() > empty_length)
      remember_alter(table, sql);
  }

  void ActionGenerateSQL::alter_table_columns_begin(db_mysql_TableRef) {
    // first_column= true;
  }

  void ActionGenerateSQL::alter_table_add_column(db_mysql_TableRef table, std::map<std::string, std::string> rename_map,
                                                 db_mysql_ColumnRef column, db_mysql_ColumnRef after) {
    if (first_change)
      first_change = false;
    else
      sql.append(",\n");

    /*
     | ADD [COLUMN] column_definition [FIRST | AFTER col_name ]
     | ADD [COLUMN] (column_definition,...)
     */

    sql.append("ADD COLUMN ");
    sql.append(generate_create(column));
    sql.append(" ");

    if (after.is_valid()) {
      // const char *after_sql= after->name().c_str();
      std::string after_name(after->name().c_str());
      std::map<std::string, std::string>::const_iterator it = rename_map.find(after_name);
      if (it != rename_map.end())
        after_name = it->second;
      sql.append("AFTER `").append(after_name).append("`");
    } else {
      sql.append("FIRST");
    }

    // return sql;
  }

  void ActionGenerateSQL::alter_table_drop_column(db_mysql_TableRef, db_mysql_ColumnRef column) {
    if (first_change)
      first_change = false;
    else
      sql.append(",\n");

    sql += ("DROP COLUMN `");
    sql += column->name().c_str();
    sql += "`";
  }

  void ActionGenerateSQL::alter_table_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col,
                                                    db_mysql_ColumnRef mod_col, db_mysql_ColumnRef after, bool modified,
                                                    std::map<std::string, std::string> column_rename_map) {
    if (first_change)
      first_change = false;
    else
      sql.append(",\n");

    /*
     | CHANGE [COLUMN] old_col_name column_definition
     [FIRST|AFTER col_name]
     */

    sql.append("CHANGE COLUMN `");
    std::map<std::string, std::string>::iterator it = column_rename_map.find(org_col->oldName().c_str());
    if (it != column_rename_map.end())
      sql.append(it->second.c_str()).append("` ");
    else
      sql.append(org_col->oldName().c_str()).append("` ");

    if (modified)
      sql.append(generate_create(org_col));
    else
      sql.append(generate_create(mod_col));
    sql.append(" ");

    if (!modified) {
      if (after.is_valid()) {
        std::string after_name(after->name().c_str());
        std::map<std::string, std::string>::const_iterator it = column_rename_map.find(after_name);
        if (it != column_rename_map.end())
          after_name = it->second;
        sql.append("AFTER `").append(after_name).append("`");
      } else {
        sql.append("FIRST");
      }
    }
  }

  void ActionGenerateSQL::alter_table_columns_end(db_mysql_TableRef) {
  }

  void ActionGenerateSQL::alter_table_indexes_begin(db_mysql_TableRef) {
  }

  void ActionGenerateSQL::alter_table_add_index(db_mysql_IndexRef index) {
    //  sql.append("\n");
    padding.pad(sql);

    if (first_change)
      first_change = false;
    else
      sql.append(",\n");

    sql.append(generate_add_index(index));
  }

  std::string ActionGenerateSQL::generate_add_index(db_mysql_IndexRef index) {
    /*
     | ADD {INDEX|KEY} [index_name] [index_type] (index_col_name,...)
     */
    return std::string("ADD ").append(generate_create(index, "", false));
  }

  void ActionGenerateSQL::alter_table_drop_index(db_mysql_IndexRef index) {
    //  sql.append("\n");
    padding.pad(sql);

    if (first_change)
      first_change = false;
    else
      sql.append(",\n");

    sql.append(generate_drop_index(index));
  }

  void ActionGenerateSQL::alter_table_change_index(db_mysql_IndexRef orgIndex, db_mysql_IndexRef newIndex) {
    auto catalog = db_CatalogRef::cast_from(orgIndex->owner()->owner()->owner());

    GrtVersionRef version;
    if (catalog->owner().is_valid())
      version = GrtVersionRef::cast_from(
        bec::getModelOption(workbench_physical_ModelRef::cast_from(catalog->owner()), "CatalogVersion"));
    else
      version = catalog->version();

    if (!bec::is_supported_mysql_version_at_least(version, 8, 0, 0)) {
      alter_table_drop_index(newIndex);
      alter_table_add_index(newIndex);
      return;
    }

    auto orgColumns = orgIndex->columns();
    auto newColumns = newIndex->columns();

    if (orgColumns.count() != newColumns.count()) {
      alter_table_drop_index(newIndex);
      alter_table_add_index(newIndex);
      return;
    }

    for (size_t i = 0; i < orgColumns.count(); i++) {
      if (orgColumns[i]->referencedColumn()->name() != newColumns[i]->referencedColumn()->name() ||
          orgColumns[i]->descend() != newColumns[i]->descend()) {
        alter_table_drop_index(newIndex);
        alter_table_add_index(newIndex);
        return;
      }
    }

    auto properties = { "algorithm", "keyBlockSize", "lockOption", "withParser", "visible", "comment" };

    std::vector<std::string> diffProps;
    for (const auto& it : properties) {
      if (orgIndex->get_member(it) != newIndex->get_member(it))
        diffProps.push_back(it);
    }

    if (diffProps.size() > 1 || (diffProps.size() == 1 && diffProps[0] != "visible")) {
      alter_table_drop_index(newIndex);
      alter_table_add_index(newIndex);
      return;
    }

    if (!indexAlter.empty()) {
      padding.pad(indexAlter);
      indexAlter.append(";\n");
    }

    if (!newIndex->oldName().empty() && newIndex->name() != newIndex->oldName()) {
      if (newIndex->isPrimary())
        return;

      indexAlter.append(strfmt("ALTER TABLE `%s`.`%s` RENAME INDEX `%s` TO `%s`;\n",
                               db_SchemaRef::cast_from(newIndex->owner()->owner())->name().c_str(),
                               db_TableRef::cast_from(newIndex->owner())->name().c_str(), newIndex->oldName().c_str(),
                               newIndex->name().c_str()));
    }

    indexAlter.append(strfmt("ALTER TABLE `%s`.`%s` ALTER INDEX `%s` %s",
                             db_SchemaRef::cast_from(newIndex->owner()->owner())->name().c_str(),
                             db_TableRef::cast_from(newIndex->owner())->name().c_str(), newIndex->name().c_str(),
                             newIndex->visible() == 1 ? "VISIBLE" : "INVISIBLE"));
  }

  void ActionGenerateSQL::alter_table_indexes_end(db_mysql_TableRef) {
    if (first_change) {
      first_change = false;
    }
    sql.append(";\n");

    sql.append(indexAlter);
  }

  void ActionGenerateSQL::alter_table_fks_begin(db_mysql_TableRef) {
    first_fk_create = true;
    first_fk_drop = true;
    fk_add_sql.clear();
    fk_drop_sql.clear();
  }

  void ActionGenerateSQL::alter_table_add_fk(db_mysql_ForeignKeyRef fk) {
    grt::StringRef ename = db_mysql_TableRef::cast_from(fk->owner())->tableEngine();
    db_mysql_StorageEngineRef engine = bec::TableHelper::get_engine_by_name(ename);
    if (engine.is_valid() && !engine->supportsForeignKeys())
      return;
    if (first_fk_create)
      first_fk_create = false;
    else
      fk_add_sql.append(",\n");

    /*
     | ADD [CONSTRAINT [symbol]]
     FOREIGN KEY [index_name] (index_col_name,...)
     [reference_definition]
     */
    fk_add_sql += "ADD ";
    fk_add_sql += global_generate_create(fk, padding, _omitSchemas);
  }

  void ActionGenerateSQL::alter_table_drop_fk(db_mysql_ForeignKeyRef fk) {
    grt::StringRef ename = db_mysql_TableRef::cast_from(fk->owner())->tableEngine();
    db_mysql_StorageEngineRef engine = bec::TableHelper::get_engine_by_name(ename);
    if (engine.is_valid() && !engine->supportsForeignKeys())
      return;

    if (first_fk_drop)
      first_fk_drop = false;
    else
      fk_drop_sql.append(",\n");

    /*
     | DROP FOREIGN KEY fk_symbol
     */
    fk_drop_sql += "DROP FOREIGN KEY `";
    fk_drop_sql += fk->name().c_str();
    fk_drop_sql += "`";
  }

  void ActionGenerateSQL::alter_table_fks_end(db_mysql_TableRef table) {
    if (!fk_add_sql.empty() && !fk_drop_sql.empty()) {
      if (!first_change)
        sql.append(",\n");
      sql.append(fk_drop_sql);
      remember_alter(table, sql); // let DROP go first
      sql.assign("ALTER TABLE ");
      sql.append(get_table_old_name(table));
      sql.append(fk_add_sql);
      comma.clear();
      first_change = false;
    } else if (!fk_add_sql.empty()) {
      if (first_change)
        first_change = false;
      else
        sql.append(",\n");
      sql.append(fk_add_sql);
    } else if (!fk_drop_sql.empty()) {
      if (first_change)
        first_change = false;
      else
        sql.append(",\n");
      sql.append(fk_drop_sql);
    }
  }

  // triggers

  static db_mysql_TriggerRef find_ordering_for_trigger(db_mysql_TriggerRef trigger, std::string& position) {
    db_mysql_TriggerRef prec;

    // the trigger FOLLOWS the last one before it
    position = "FOLLOWS";

    db_mysql_TableRef table(db_mysql_TableRef::cast_from(trigger->owner()));
    GRTLIST_FOREACH(db_mysql_Trigger, table->triggers(), t) {
      if ((*t)->event() == trigger->event() && (*t)->timing() == trigger->timing()) {
        if (trigger == *t) {
          if (prec.is_valid())
            break;
          else {
            // if there are no triggers before it, then maybe there's something after it
            position = "PRECEDES";
            continue;
          }
        }
        prec = *t;
        // first one to match after the trigger itself is good to go
        if (position == "PRECEDES")
          break;
      }
    }
    return prec;
  }

  void ActionGenerateSQL::create_trigger(db_mysql_TriggerRef trigger, bool for_alter) {
    std::string trigger_sql;
    std::string schema_name = trigger->owner()->owner()->name().c_str();
    if (!_omitSchemas || _gen_use)
      trigger_sql.append("USE `").append(schema_name).append("`").append(_non_std_sql_delimiter).append("\n");

    std::string trigger_definition = trigger->sqlDefinition();

    if (for_alter) {
      std::string position;

      // if we're altering (ie inserting a new trigger), we need to find out what's the previous trigger of the same
      // type
      // and then rewrite the trigger code to add the FOLLOWS keyword (5.7+)
      db_mysql_TriggerRef preceding = find_ordering_for_trigger(trigger, position);
      if (preceding.is_valid()) {
        // check if the remember() at the end of this method was called for the "preceding" object
        bool flag = false;
        if (target_list.is_valid()) {
          if (target_object_list.get_index(preceding) != grt::BaseListRef::npos)
            flag = true;
        } else {
          if (target_map
                .get(_use_oids_as_dict_key ? preceding.id() : get_full_object_name_for_key(preceding, _case_sensitive))
                .is_valid())
            flag = true;
        }
        if (!flag) {
          trigger_definition = "CREATE";
          if (!trigger->definer().empty()) {
            std::string definer = trigger->definer();
            // workaround for bug in parser, where definers get their outer `` stripped
            if (definer[0] != '`' && definer.find("`@") != std::string::npos)
              definer = "`" + definer;
            if (definer[definer.size() - 1] != '`' && definer.find("@`") != std::string::npos)
              definer = definer + "`";
            trigger_definition.append(" ").append("DEFINER = ").append(definer);
          }
          if (_omitSchemas)
            trigger_definition.append(" TRIGGER `").append(trigger->name()).append("`");
          else
            trigger_definition.append(" TRIGGER `")
              .append(schema_name)
              .append("`.`")
              .append(trigger->name())
              .append("`");
          trigger_definition.append(" ").append(trigger->timing());
          trigger_definition.append(" ").append(trigger->event());
          trigger_definition.append(" ").append("ON `").append(trigger->owner()->name()).append("`");
          trigger_definition.append(" ").append("FOR EACH ROW");
          trigger_definition.append(" ").append(position).append(" `").append(preceding->name()).append("`");
          trigger_definition.append("\n").append(trigger->sqlBody());
        }
      }
    }

    trigger_sql.append(trigger_definition);

    //  if(for_alter)
    //    remember_alter(trigger, trigger_sql);
    //  else
    remember(trigger, trigger_sql);
  }

  void ActionGenerateSQL::drop_trigger(db_mysql_TriggerRef trigger, bool for_alter) {
    std::string trigger_sql;
    if (!_omitSchemas || _gen_use)
      trigger_sql.append("USE `")
        .append(trigger->owner()->owner()->name().c_str())
        .append("`")
        .append(_non_std_sql_delimiter)
        .append("\n");
    trigger_sql.append("DROP TRIGGER IF EXISTS ").append(get_name(trigger)).append(" ");
    if (for_alter)
      remember_alter(trigger, trigger_sql);
    else
      remember(trigger, trigger_sql);
  }

  // views

  void ActionGenerateSQL::create_view(db_mysql_ViewRef view) {
    bool or_replace_present = false;

    std::string view_def;
    view_def.append(view->sqlDefinition().c_str());

    std::vector<std::string> lines = base::split(view_def, "\n");
    auto flag = std::regex::ECMAScript | std::regex::icase;
    for (auto& line : lines) {
      std::regex pattern("^\\s*CREATE\\s+OR\\s+REPLACE\\s+", flag);
      std::smatch itemsMatch;
      if (std::regex_search(line, itemsMatch, pattern) && itemsMatch.size() > 0) {
        or_replace_present = true;
        break;
      }
      if (!or_replace_present) {
        pattern = std::regex("^\\s*CREATE\\s+", flag);
        if (std::regex_search(line, itemsMatch, pattern) && itemsMatch.size() > 0) {
          std::ssub_match subMatch = itemsMatch[0];
          line.insert(itemsMatch.prefix().str().size() + subMatch.str().size(), " OR REPLACE ");
          view_def = base::join(lines, "\n");
          break;
        }
      }
    }
    if (_omitSchemas) {
      SqlFacade* parser = SqlFacade::instance_for_rdbms_name("Mysql");
      Sql_schema_rename::Ref renamer = parser->sqlSchemaRenamer();
      renamer->rename_schema_references(view_def, view->owner()->name(), "");
    }
    if (!_omitSchemas || _gen_use) {
      std::string use_def;
      use_def.append("USE `").append(view->owner()->name()).append("`;\n");
      use_def.append(view_def);
      view_def = use_def;
    }
    remember(view, view_def);
  }

  void ActionGenerateSQL::drop_view(db_mysql_ViewRef view) {
    std::string view_sql;
    view_sql.append("DROP VIEW IF EXISTS ").append(get_name(view)).append(" ");
    remember(view, view_sql);
  }

  // routines
  void ActionGenerateSQL::create_routine(db_mysql_RoutineRef routine, bool for_alter) {
    std::string routine_sql;
    routine_sql = "\nDELIMITER ";
    routine_sql.append(_non_std_sql_delimiter).append("\n");

    if (!_omitSchemas || _gen_use) {
      routine_sql.append("USE `");
      routine_sql.append(routine->owner()->name()).append("`").append(_non_std_sql_delimiter).append("\n");
    }
    routine_sql.append(routine->sqlDefinition().c_str()).append(_non_std_sql_delimiter).append("\n");

    if (_omitSchemas) {
      SqlFacade* parser = SqlFacade::instance_for_rdbms_name("Mysql");
      Sql_schema_rename::Ref renamer = parser->sqlSchemaRenamer();
      renamer->rename_schema_references(routine_sql, routine->owner()->name(), "");
    }

    routine_sql.append("\nDELIMITER ;\n");

    // remove_delims(routine_sql);
    if (for_alter)
      remember_alter(routine, routine_sql);
    else
      remember(routine, routine_sql);
  }

  void ActionGenerateSQL::drop_routine(db_mysql_RoutineRef routine, bool for_alter) {
    std::string routine_sql;

    if (!_omitSchemas || _gen_use) {
      routine_sql = "\nUSE `";
      routine_sql.append(routine->owner()->name()).append("`;\n");
    }

    routine_sql.append("DROP ")
      .append(routine->routineType().c_str())
      .append(" IF EXISTS ")
      .append(get_name(routine))
      .append(";\n");

    if (for_alter)
      remember_alter(routine, routine_sql);
    else
      remember(routine, routine_sql);
  }

  // users
  void ActionGenerateSQL::create_user(db_UserRef user) {
    std::string sql;

    sql.append("CREATE USER ").append(quote_user(user->name()));
    if (user->password().is_valid() && *user->password().c_str())
      sql.append(" IDENTIFIED BY '").append(*user->password()).append("'");

    sql.append(";\n\n");

    std::list<std::string> grants;
    gen_grant_sql(db_CatalogRef::cast_from(user->owner()), user, grants, _omitSchemas);

    std::list<std::string>::iterator iter = grants.begin();
    for (; iter != grants.end(); ++iter)
      sql.append(*iter).append(";\n");

    remember(user, sql);
  }

  void ActionGenerateSQL::drop_user(db_UserRef user) {
    auto catalog = db_CatalogRef::cast_from(user->owner());

    GrtVersionRef version;
    if (catalog->owner().is_valid())
      version = GrtVersionRef::cast_from(
        bec::getModelOption(workbench_physical_ModelRef::cast_from(catalog->owner()), "CatalogVersion", true));
    else
      version = catalog->version();

    if (bec::is_supported_mysql_version_at_least(version, 5, 7, 0)) {
      sql = "DROP USER IF EXISTS " + *user->name();
    } else {
      // Before 5.7 there was no IF EXISTS clause. So we use the implicit user creation with grant here.
      sql = "GRANT USAGE ON *.* TO " + *user->name() + ";\n DROP USER " + *user->name();
    }
    remember(user, sql);
  }

  void ActionGenerateSQL::remember(const GrtNamedObjectRef& obj, const std::string& sql, const bool front) {
    if (target_list.is_valid()) {
      if (disable_object_list)
        return;
      target_list.insert(grt::StringRef(sql), front ? 0 : (size_t)StringListRef::npos);
      if (target_object_list.is_valid())
        target_object_list.insert(obj, front ? 0 : (size_t)StringListRef::npos);
    } else {
      target_map.set(_use_oids_as_dict_key ? obj.id() : get_full_object_name_for_key(obj, _case_sensitive),
                     grt::StringRef(sql));
    }
  }

  // in case of ALTERs there could be > 1 statement to remember
  // so we use grt::StringListRefs as needed
  void ActionGenerateSQL::remember_alter(const GrtNamedObjectRef& obj, const std::string& sql) {
    if (target_list.is_valid()) {
      if (disable_object_list)
        return;
      target_list.insert(grt::StringRef(sql));
      if (target_object_list.is_valid())
        target_object_list.insert(obj);
      return;
    }

    std::string key = _use_oids_as_dict_key ? obj.id() : get_full_object_name_for_key(obj, _case_sensitive);

    if (target_map.has_key(key)) {
      grt::ValueRef value = target_map.get(key);
      if (grt::StringRef::can_wrap(value)) {
        grt::StringListRef list_value(grt::Initialized);
        list_value.insert(grt::StringRef::cast_from(value));
        list_value.insert(grt::StringRef(sql));
        target_map.set(key, list_value);
      } else if (grt::StringListRef::can_wrap(value)) {
        grt::StringListRef::cast_from(value).insert(grt::StringRef(sql));
      } else {
        // a bug
      }
    } else {
      target_map.set(key, grt::StringRef(sql));
    }
  }

} // namespace

DbMySQLImpl::DbMySQLImpl(grt::CPPModuleLoader* ldr) : grt::ModuleImplBase(ldr), _default_traits(true) {
  _default_traits.set("version", grt::StringRef("8.0.5"));
  _default_traits.set("CaseSensitive", grt::IntegerRef(1));
  _default_traits.set("maxTableCommentLength", grt::IntegerRef(2048));
  _default_traits.set("maxIndexCommentLength", grt::IntegerRef(1024));
  _default_traits.set("maxColumnCommentLength", grt::IntegerRef(1024));
}

ssize_t DbMySQLImpl::generateSQL(GrtNamedObjectRef org_object, const grt::DictRef& options,
                                 std::shared_ptr<DiffChange> changes) {
  grt::ValueRef result = options.get("OutputContainer");
  grt::ListRef<GrtNamedObject> obj_list;
  grt::DictRef dbsettings = grt::DictRef::cast_from(options.get("DBSettings", getDefaultTraits()));

  if (options.has_key("OutputObjectContainer"))
    obj_list = grt::ListRef<GrtNamedObject>::cast_from(options.get("OutputObjectContainer"));
  if (result.type() == DictType) {
    ActionGenerateSQL generator =
      ActionGenerateSQL(result, obj_list, dbsettings, options.get_int("UseOIDAsResultDictKey", 0) != 0);
    DiffSQLGeneratorBE(options, dbsettings, &generator)
      .process_diff_change(org_object, changes.get(), grt::DictRef::cast_from(result));
  } else if (result.type() == ListType) {
    ActionGenerateSQL generator =
      ActionGenerateSQL(result, obj_list, dbsettings, options.get_int("UseOIDAsResultDictKey", 0) != 0);
    DiffSQLGeneratorBE(options, dbsettings, &generator)
      .process_diff_change(org_object, changes.get(), grt::StringListRef::cast_from(result), obj_list);
  }

  return 0;
}

grt::StringRef DbMySQLImpl::generateReport(GrtNamedObjectRef org_object, const grt::DictRef& options,
                                           std::shared_ptr<DiffChange> changes) {
  grt::StringRef tpl_file = grt::StringRef::cast_from(options.get("TemplateFile"));

  {
    ActionGenerateReport r(tpl_file);

    DiffSQLGeneratorBE(options, grt::DictRef::cast_from(options.get("DBSettings", getDefaultTraits())), &r)
      .process_diff_change(org_object, changes.get(), grt::StringListRef(), grt::ListRef<GrtNamedObject>());

    grt::StringRef retval(r.generate_output());

    return retval;
  }
}

grt::StringRef DbMySQLImpl::generateReportForDifferences(GrtNamedObjectRef org_object, GrtNamedObjectRef oth_object,
                                                         const grt::DictRef& options) {
  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = (unsigned int)options.get_int("OMFDontDiffMask", omf.dontdiff_mask);
  grt::NormalizedComparer normalizer;
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> alter_change = diff_make(org_object, oth_object, &omf);

  grt::StringRef tpl_file = grt::StringRef::cast_from(options.get("TemplateFile"));

  {
    if (alter_change == NULL) // There are no changes, user probably selected the same schema on the same instance.
      return grt::StringRef("");

    ActionGenerateReport r(tpl_file);

    DiffSQLGeneratorBE(options, grt::DictRef::cast_from(options.get("DBSettings", getDefaultTraits())), &r)
      .process_diff_change(org_object, alter_change.get(), grt::StringListRef(), grt::ListRef<GrtNamedObject>());

    grt::StringRef retval(r.generate_output());

    return retval;
  }
}

grt::DictRef DbMySQLImpl::generateSQLForDifferences(GrtNamedObjectRef srcobj, GrtNamedObjectRef dstobj,
                                                    grt::DictRef options) {
  grt::DictRef sql_map(true);

  default_omf omf;
  grt::NormalizedComparer normalizer;
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> changes = diff_make(srcobj, dstobj, &omf);

  options.set("DiffCaseSensitiveness", grt::IntegerRef(normalizer.is_case_sensitive()));

  // the SQL generator has a weird inverted logic where filter lists are used by default
  // even if they're not set
  if (!options.has_key("UseFilteredLists"))
    options.gset("UseFilteredLists", 0);

  if (changes) {
    options.set("OutputContainer", sql_map);
    generateSQL(srcobj, options, changes);
  }

  return sql_map;
}

static bool exists_in_map(const GrtNamedObjectRef& object, const DictRef& dict, const bool case_sensitive) {
  std::string qname(get_full_object_name_for_key(object, case_sensitive));
  return dict.has_key(qname);
}

static std::string string_from_map(const GrtNamedObjectRef& object, const DictRef& dict, const bool case_sensitive) {
  std::string qname(get_full_object_name_for_key(object, case_sensitive));
  StringRef res = dict.get_string(qname);

  // if (!res.is_valid())
  //  return std::string();
  return *res;
}

static std::string reformat_text_for_comment(const std::string& text) {
  if (text.empty())
    return "";
  std::string comment = text;
  base::replaceStringInplace(comment, "\n", "\n-- ");
  return "-- " + comment + "\n";
}

class TableSorterByFK {
  std::set<db_mysql_TableRef> generated_tables;

public:
  void perform(db_mysql_TableRef table, std::vector<db_mysql_TableRef>& result) {
    if (table->modelOnly() || table->isStub() || (generated_tables.find(table) != generated_tables.end()))
      return;
    generated_tables.insert(table);
    const grt::ListRef<db_mysql_ForeignKey> fks = table->foreignKeys();
    for (size_t c = fks.count(), i = 0; i < c; i++) {
      const db_mysql_ForeignKeyRef fk = fks.get(i);
      if (fk.is_valid() && fk->referencedTable().is_valid() && !fk->modelOnly())
        perform(fk->referencedTable(), result);
    }
    result.push_back(table);
  }
};

class SQLComposer {
protected:
  std::string sql_mode;
  std::string non_std_sql_delimiter;
  ;
  bool show_warnings;
  bool _omitSchemas;
  bool no_view_placeholder;
  bool _case_sensitive;
  grt::DictRef _decomposer_options;
  bool include_scripts;
  bool include_document_properties;
  typedef std::map<std::string, std::vector<std::pair<std::string, std::string> > > alias_map_t;
  alias_map_t alias_map;

  SQLComposer(const grt::DictRef options) : _case_sensitive(false) {
    sql_mode = options.get_string("SQL_MODE",
                                  "ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_"
                                  "DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION");
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms_name("Mysql");
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
    non_std_sql_delimiter = bec::GRTManager::get()->get_app_option_string("SqlDelimiter", "$$");
    show_warnings = options.get_int("GenerateWarnings") != 0;
    _omitSchemas = options.get_int("OmitSchemas") != 0;
    no_view_placeholder = options.get_int("NoViewPlaceholders") != 0;

    grt::ValueRef dboptsval = options.get("DBSettings");
    if (dboptsval.is_valid() && grt::DictRef::can_wrap(dboptsval)) {
      grt::DictRef opts = grt::DictRef::cast_from(dboptsval);
      if (opts.is_valid()) {
        _decomposer_options = grt::DictRef(true);
        _decomposer_options.set("case_sensitive_identifiers", grt::IntegerRef(opts.get_int("CaseSensitive") != 0));
      }
    }
    if (!_decomposer_options.is_valid()) {
      ssize_t case_sensitive_opt = options.get_int("CaseSensitive", -1);
      if (case_sensitive_opt != -1) {
        _decomposer_options = grt::DictRef(true);
        _decomposer_options.set("case_sensitive_identifiers", grt::IntegerRef(case_sensitive_opt ? 1 : 0));
      }
    }
    include_document_properties = options.get_int("GenerateDocumentProperties", 1) != 0;
    include_scripts = options.get_int("GenerateAttachedScripts") != 0;
  };

  void send_output(const std::string& msg) const {
    grt::GRT::get()->send_output(msg);
  };

  std::string show_warnings_sql() const {
    return show_warnings ? "SHOW WARNINGS;\n" : "";
  }

  std::string set_server_vars() const {
    std::string result;
    result.append("SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;\n");
    result.append("SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;\n");
    result.append(base::sqlstring("SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE=?;\n\n", 0) << sql_mode);
    return result;
  }

  std::string restore_server_vars() const {
    std::string result;

    result.append("\n");
    result.append("SET SQL_MODE=@OLD_SQL_MODE;\n");
    result.append("SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;\n");
    result.append("SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;\n");
    return result;
  }

  std::string generate_view_placeholder(const db_mysql_ViewRef view) {
    std::string sql;
    std::string view_q_name(get_name(view, _omitSchemas));

    SelectStatement::Ref select_statement(new SelectStatement());
    SqlFacade* parser = SqlFacade::instance_for_rdbms_name("Mysql");
    parser->sqlStatementDecomposer(_decomposer_options)->decompose_view(view, select_statement);

    sql.append("\n-- -----------------------------------------------------\n")
      .append("-- Placeholder table for view ")
      .append(view_q_name)
      .append("\n-- -----------------------------------------------------\n");

    sql.append("CREATE TABLE IF NOT EXISTS ");
    sql.append(view_q_name).append(" (");
    if (select_statement->select_items.empty())
      sql.append("`id` INT");
    else {
      std::vector<std::string> used_colnames;
      used_colnames.reserve(select_statement->select_items.size());

      bool first_col = true;
      for (SelectItems::const_iterator it = select_statement->select_items.begin();
           it != select_statement->select_items.end(); ++it)
        if (std::find(used_colnames.begin(), used_colnames.end(), it->effective_alias()) == used_colnames.end()) {
          std::string alias_name = it->effective_alias();
          if (alias_name.size() > 64) {
            std::string new_name = get_name_suggestion(
              std::bind(std::not_equal_to<std::vector<std::string>::iterator>(),
                        std::bind(std::find<std::vector<std::string>::iterator, std::string>, used_colnames.begin(),
                                  used_colnames.end(), std::placeholders::_1),
                        used_colnames.end()),
              "Col_placeholder", true);
            alias_map[view.id()].push_back(std::pair<std::string, std::string>(new_name, alias_name));
            alias_name = new_name;
          };

          if (!first_col)
            sql.append(", ");
          else
            first_col = false;
          sql.append("`").append(alias_name).append("` INT");
          used_colnames.push_back(alias_name);
        }
    }
    sql.append(");\n");

    sql.append(show_warnings_sql());
    return sql;
  }

  std::string generate_view_ddl(const db_mysql_ViewRef view, std::string create_view, std::string drop_view = "") {
    std::string sql;
    std::string view_q_name(get_name(view, _omitSchemas));

    // create view
    sql.append("\n");
    sql.append("-- -----------------------------------------------------\n");
    sql.append("-- View ").append(view_q_name).append("\n");
    sql.append("-- -----------------------------------------------------\n");

    // Important: first delete the placeholder then the view. Otherwise we can get a server error
    // (which seems inconsistent, but that's how it is).
    if (!no_view_placeholder) {
      // remove placehoder
      sql.append("DROP TABLE IF EXISTS ").append(view_q_name).append(";\n");
      sql.append(show_warnings_sql());
    }

    if (!drop_view.empty())
      sql.append(drop_view).append(";\n").append(show_warnings_sql());

    // view DDL
    if (!create_view.empty()) {
      if (alias_map.find(view.id()) != alias_map.end()) {
        std::string create_view_with_aliases = create_view;
        size_t idx = 0;
        const alias_map_t::mapped_type aliases = alias_map[view.id()];
        for (alias_map_t::mapped_type::const_iterator It = aliases.begin(); It != aliases.end(); ++It) {
          idx = create_view_with_aliases.find(It->second, idx);
          if (idx == std::string::npos)
            continue;
          idx += It->second.size();
          std::string alias(" AS '");
          alias.append(It->first).append("' ");
          create_view_with_aliases.insert(idx, alias);
          idx += alias.size();
        }
        sql.append(create_view_with_aliases);
      } else
        sql.append(create_view);

      if (!base::hasSuffix(base::trim_right(create_view, "\n"), ";"))
        sql.append(";");
      sql.append("\n");
    }
    sql.append(show_warnings_sql());
    return sql;
  }

  std::string user_script(const db_ScriptRef& script) const {
    std::string out_sql;
    out_sql.append("-- begin attached script '").append(script->name()).append("'\n");
    out_sql.append(script->getText()).append("\n");
    out_sql.append("-- end attached script '").append(script->name()).append("'\n");
    return out_sql;
  }
};

//----------------------------------------------------------------------------------------------------------------------

static std::string generateDocumentProperties(const db_CatalogRef cat) {
  std::string output;
  if (cat->owner().is_valid() && cat->owner()->owner().is_valid()) {
    output.append("-- Generated: ").append(fmttime(0, DATETIME_FMT)).append("\n");

    workbench_DocumentRef doc(workbench_DocumentRef::cast_from(cat->owner()->owner()));
    if (strlen(doc->info()->caption().c_str()))
      output.append("-- Model: ").append(doc->info()->caption()).append("\n");
    if (strlen(doc->info()->version().c_str()))
      output.append("-- Version: ").append(doc->info()->version()).append("\n");
    if (strlen(doc->info()->project().c_str()))
      output.append("-- Project: ").append(doc->info()->project()).append("\n");
    if (strlen(doc->info()->author().c_str()))
      output.append("-- Author: ").append(doc->info()->author()).append("\n");
    if (strlen(doc->info()->description().c_str())) {
      std::string description = doc->info()->description();
      base::replaceStringInplace(description, "\n", "\n-- ");
      output.append("-- ").append(description).append("\n");
    }
  }

  return output;
}

//----------------------------------------------------------------------------------------------------------------------

class SQLExportComposer : public SQLComposer {
  bool gen_create_index;
  bool gen_use;
  bool gen_drops;
  bool gen_schema_drops;
  bool no_user_just_privileges;
  bool gen_inserts;
  bool caseSensitive;
  bool no_view_placeholders;
  bool no_FK_for_inserts;
  bool triggers_after_inserts;
  bool sortTablesAlphabetically;
  grt::DictRef create_map;
  grt::DictRef drop_map;

public:
  SQLExportComposer(const grt::DictRef options, grt::DictRef cmap, grt::DictRef dmap)
    : SQLComposer(options), create_map(cmap), drop_map(dmap) {
    gen_create_index = options.get_int("GenerateCreateIndex") != 0;
    gen_use = options.get_int("GenerateUse") != 0;
    gen_drops = options.get_int("GenerateDrops") != 0;
    gen_schema_drops = options.get_int("GenerateSchemaDrops") != 0;
    no_user_just_privileges = options.get_int("NoUsersJustPrivileges") != 0;
    no_view_placeholders = options.get_int("NoViewPlaceholders") != 0;
    gen_inserts = options.get_int("GenerateInserts") != 0;
    caseSensitive = options.get_int("CaseSensitive") != 0;
    no_FK_for_inserts = options.get_int("NoFKForInserts") != 0;
    triggers_after_inserts = options.get_int("TriggersAfterInserts") != 0;
    sortTablesAlphabetically = options.get_int("SortTablesAlphabetically") != 0;
  }

protected:
  std::string schemata_sql(const grt::ListRef<db_mysql_Schema>& schemata) const {
    std::string result;
    for (size_t c1 = schemata.count(), i = 0; i < c1; i++) {
      db_mysql_SchemaRef schema = schemata.get(i);
      if (schema->modelOnly())
        continue;

      std::string comment = reformat_text_for_comment(schema->comment());
      result.append("-- -----------------------------------------------------\n");
      result.append("-- Schema ").append(schema->name()).append("\n");
      result.append("-- -----------------------------------------------------\n");
      result.append(comment);

      if ((!_omitSchemas || gen_use) && (create_map.has_key(get_full_object_name_for_key(schema, caseSensitive)))) {
        if (gen_schema_drops)
          result.append("DROP SCHEMA IF EXISTS `").append(schema->name().c_str()).append("` ;\n");

        std::string comment = schema->comment();
        result.append("\n");
        result.append("-- -----------------------------------------------------\n");
        result.append("-- Schema ").append(schema->name()).append("\n");
        if (!comment.empty()) {
          result.append("--\n");
          base::replaceStringInplace(comment, "\n", "\n-- ");
          result.append("-- ").append(comment).append("\n");
        }
        result.append("-- -----------------------------------------------------\n");
        result.append(string_from_map(schema, create_map, caseSensitive)).append(";\n");
      }
      result.append(show_warnings_sql());
    }
    return result;
  };

  std::string table_sql(const db_mysql_TableRef table) const {
    std::string result;
    std::string create_table_sql = string_from_map(table, create_map, caseSensitive);

    result.append("\n");
    result.append("-- -----------------------------------------------------\n");
    result.append("-- Table ").append(get_name(table, _omitSchemas)).append("\n");
    result.append("-- -----------------------------------------------------\n");

    if (gen_drops)
      result.append(string_from_map(table, drop_map, caseSensitive)).append(";\n\n").append(show_warnings_sql());

    result.append(create_table_sql).append(";\n\n");
    result.append(show_warnings_sql());
    send_output(
      std::string("Processing Table ").append(table->owner()->name()).append(".").append(table->name()).append("\n"));

    // table indices
    if (gen_create_index) {
      grt::ListRef<db_mysql_Index> indices = table->indices();
      for (size_t c3 = indices.count(), k = 0; k < c3; k++) {
        std::string index_sql = string_from_map(indices.get(k), create_map, caseSensitive);
        if (!index_sql.empty())
          result.append(index_sql).append(";\n\n").append(show_warnings_sql());
      }
    }
    return result;
  }

  std::string table_inserts_sql(const db_mysql_TableRef table) const {
    std::string result;
    std::string use_code;
    if (!_omitSchemas || gen_use)
      use_code.append("USE `").append(table->owner()->name().c_str()).append("`;\n");

    std::string table_inserts_sql;
    {
      Recordset_table_inserts_storage::Ref input_storage = Recordset_table_inserts_storage::create();
      input_storage->table(table);

      Recordset::Ref rs = Recordset::create();
      rs->data_storage(input_storage);
      rs->reset();

      Recordset_sql_storage::Ref output_storage = Recordset_sql_storage::create();
      output_storage->table_name(table->name());
      output_storage->rdbms(db_mgmt_RdbmsRef::cast_from(
        table->owner() /*schema*/->owner() /*catalog*/->owner() /*phys.model*/->get_member("rdbms")));
      output_storage->schema_name(table->owner()->name());
      output_storage->omit_schema_qualifier(_omitSchemas);
      output_storage->binding_blobs(false);
      output_storage->serialize(rs);
      table_inserts_sql = output_storage->sql_script();
    }
    if (table_inserts_sql.empty())
      return table_inserts_sql;
    result.append("\n-- -----------------------------------------------------\n-- Data for table ")
      .append(get_name(table, _omitSchemas))
      .append("\n-- -----------------------------------------------------\nSTART TRANSACTION;\n")
      .append(use_code)
      .append(table_inserts_sql)
      .append("\nCOMMIT;\n");
    return result;
  }

  std::string view_placeholder(const db_mysql_ViewRef view) {
    if (view->modelOnly())
      return "";
    if (exists_in_map(view, create_map, caseSensitive))
      return generate_view_placeholder(view);
    return "";
  }

  std::string routine_sql(const db_mysql_RoutineRef routine) const {
    std::string result;
    send_output(std::string("Processing Routine ")
                  .append(routine->owner()->name())
                  .append(".")
                  .append(routine->name())
                  .append("\n"));

    if (routine->modelOnly())
      return "";
    std::string create_routine_sql = string_from_map(routine, create_map, caseSensitive);
    if (create_routine_sql.empty())
      return "";

    result.append("\n");
    result.append("-- -----------------------------------------------------\n");
    result.append("-- ")
      .append(routine->routineType().c_str())
      .append(" ")
      .append(routine->name().c_str())
      .append("\n");
    result.append("-- -----------------------------------------------------\n");

    std::string drop_string = string_from_map(routine, drop_map, caseSensitive);
    if (!drop_string.empty())
      result.append(drop_string).append(show_warnings_sql());

    std::string create_string = string_from_map(routine, create_map, caseSensitive);
    if (!create_string.empty())
      result.append(create_string).append(show_warnings_sql());

    return result;
  }

  std::string view_sql(const db_mysql_ViewRef view) {
    send_output(
      std::string("Processing View ").append(view->owner()->name()).append(".").append(view->name()).append("\n"));

    if (view->modelOnly() || !exists_in_map(view, create_map, caseSensitive))
      return "";

    return generate_view_ddl(view, string_from_map(view, create_map, caseSensitive),
                             string_from_map(view, drop_map, caseSensitive));
  }

  std::string trigger_sql(const db_mysql_TriggerRef trigger) const {
    std::string result;

    send_output(std::string("Processing Trigger ")
                  .append(trigger->owner()->owner()->name())
                  .append(".")
                  .append(trigger->owner()->name())
                  .append(".")
                  .append(trigger->name())
                  .append("\n"));

    if (trigger->modelOnly() || !exists_in_map(trigger, create_map, caseSensitive))
      return "";

    // if(gen_drops)
    {
      std::string drop_trigger(string_from_map(trigger, drop_map, caseSensitive));
      if (!drop_trigger.empty())
        result.append("\n").append(drop_trigger).append(non_std_sql_delimiter).append("\n");
      if (show_warnings)
        result.append("SHOW WARNINGS").append(non_std_sql_delimiter).append("\n");
    }
    result.append(string_from_map(trigger, create_map, caseSensitive)).append(non_std_sql_delimiter).append("\n\n");
    if (show_warnings)
      result.append("SHOW WARNINGS").append(non_std_sql_delimiter).append("\n");

    return result;
  }

  std::string user_sql(const db_UserRef user) const {
    std::string result;
    if (user->modelOnly() || !exists_in_map(user, create_map, caseSensitive))
      return "";

    std::string create_user_sql = string_from_map(user, create_map, caseSensitive);

    // if(gen_drops)
    if (exists_in_map(user, drop_map, caseSensitive)) {
      // There is no DROP USER IF EXISTS clause so we create one with
      // GRANT which will fail in traditional mode due to NO_AUTO_CREATE_USER
      result.append("SET SQL_MODE = '';\n");
      result.append(string_from_map(user, drop_map, caseSensitive)).append(";\n");
      result.append(base::sqlstring("SET SQL_MODE=?;\n", 0) << sql_mode).append(show_warnings_sql());
    }
    result.append(string_from_map(user, create_map, caseSensitive)).append(show_warnings_sql());
    send_output(std::string("Processing User ").append(user->name()).append("\n"));
    return result;
  }

public:
  std::string get_export_sql(const db_mysql_CatalogRef cat) {
    std::string out_sql;
    std::string inserts_sql;  // separate from main sql script & append to it as a last step,
                              // to separate creation of structures from data loading.
    std::string triggers_sql; // Triggers DDLs could be prior or after INSERTs depending on settings

    out_sql.append("-- MySQL Workbench Forward Engineering").append("\n");
    if (include_document_properties)
      out_sql.append(generateDocumentProperties(db_CatalogRef::cast_from(cat)));
    out_sql.append("\n");

    if (include_scripts && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->forwardEngineerScriptPosition() == "top_file")
          out_sql.append(user_script(*script));
      }
    }

    send_output("Generating Script\n");
    out_sql.append(set_server_vars());
    TableSorterByFK sorter;

    if (include_scripts && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->forwardEngineerScriptPosition() == "before_ddl")
          out_sql.append(user_script(*script));
      }
    }

    // schemata
    grt::ListRef<db_mysql_Schema> schemata = cat->schemata();
    out_sql.append(schemata_sql(schemata));
    for (size_t c1 = schemata.count(), i = 0; i < c1; i++) {
      std::string objects_sql;
      std::string schema_triggers_sql;

      db_mysql_SchemaRef schema = schemata.get(i);
      if (schema->modelOnly())
        continue;

      send_output(std::string("Processing Schema ").append(schema->name()).append("\n"));

      if ((!_omitSchemas || gen_use) && (create_map.has_key(get_full_object_name_for_key(schema, caseSensitive))))
        out_sql.append("USE `").append(schema->name().c_str()).append("` ;\n");

      // tables
      grt::ListRef<db_mysql_Table> tables = schema->tables();
      std::vector<db_mysql_TableRef> sortedTables;
      if (sortTablesAlphabetically) {
        sortedTables.reserve(tables.count());
        for (const auto& it : tables) {
          sortedTables.push_back(it);
        }

        std::sort(sortedTables.begin(), sortedTables.end(), [&](db_mysql_TableRef& first, db_mysql_TableRef& second) {
          return base::string_compare(first->name(), second->name(), caseSensitive) < 0 ? true : false;
        });
      } else {
        for (size_t c2 = tables.count(), j = 0; j < c2; j++)
          sorter.perform(tables.get(j), sortedTables);
      }

      for (std::vector<db_mysql_TableRef>::iterator It = sortedTables.begin(); It != sortedTables.end(); ++It) {
        db_mysql_TableRef table = *It;
        if (table->modelOnly() || table->isStub())
          continue;
        if (exists_in_map(table, create_map, caseSensitive)) {
          out_sql.append(table_sql(table));
          if (gen_inserts) {
            std::string tmp = table_inserts_sql(table);
            if (!tmp.empty())
              inserts_sql.append(tmp).append("\n");
          }
        } // process table

        // Fill triggers_sql with triggers DDLs and append it to out_sql later
        // tables
        grt::ListRef<db_mysql_Trigger> triggers = table->triggers();
        for (size_t c3 = triggers.count(), k = 0; k < c3; k++)
          schema_triggers_sql.append(trigger_sql(triggers.get(k)));
      }
      if (!schema_triggers_sql.empty()) {
        if (!_omitSchemas || gen_use)
          triggers_sql.append("USE `").append(schema->name().c_str()).append("`;\n");
        triggers_sql.append("\nDELIMITER ").append(non_std_sql_delimiter).append("\n");
        triggers_sql.append(schema_triggers_sql);
        triggers_sql.append("\nDELIMITER ;\n");
      }
    }

    for (size_t c1 = schemata.count(), i = 0; i < c1; i++) {
      std::string objects_sql;
      db_mysql_SchemaRef schema = schemata.get(i);
      if (schema->modelOnly())
        continue;

      send_output(std::string("Processing Schema ").append(schema->name()).append("\n"));

      grt::ListRef<db_mysql_View> views = schema->views();
      // views placeholder tables
      if (!no_view_placeholders) {
        for (size_t c2 = views.count(), j = 0; j < c2; j++)
          objects_sql.append(view_placeholder(views.get(j)));
      }

      // routines
      grt::ListRef<db_mysql_Routine> routines = schema->routines();
      if (schema->routines().count() > 0)
        for (size_t c2 = routines.count(), j = 0; j < c2; j++)
          objects_sql.append(routine_sql(routines.get(j)));

      // views DDL
      for (size_t c2 = views.count(), j = 0; j < c2; j++)
        objects_sql.append(view_sql(views.get(j)));

      if (!objects_sql.empty() && create_map.has_key(get_full_object_name_for_key(schema, caseSensitive))) {
        if (!_omitSchemas || gen_use)
          out_sql.append("USE `").append(schema->name().c_str()).append("` ;\n");
        out_sql.append(objects_sql);
      }
    }

    if (!triggers_after_inserts)
      out_sql.append(triggers_sql);

    if (no_user_just_privileges) {
      std::list<std::string> grants;
      gen_grant_sql(cat, grants);

      for (std::list<std::string>::iterator iter = grants.begin(); iter != grants.end(); ++iter)
        out_sql.append(*iter).append(";\n");
    } else {
      grt::ListRef<db_User> users = cat->users();
      for (size_t c1 = users.count(), i = 0; i < c1; i++)
        out_sql.append(user_sql(users.get(i)));
    }

    if (!no_FK_for_inserts)
      out_sql.append(restore_server_vars());

    if (gen_inserts && !inserts_sql.empty()) {
      if (include_scripts && cat->owner().is_valid()) {
        GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
          if ((*script)->forwardEngineerScriptPosition() == "before_inserts")
            out_sql.append(user_script(*script));
        }
      }

      out_sql.append(inserts_sql);

      if (include_scripts && cat->owner().is_valid()) {
        GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
          if ((*script)->forwardEngineerScriptPosition() == "after_inserts")
            out_sql.append(user_script(*script));
        }
      }
    }

    if (triggers_after_inserts)
      out_sql.append(triggers_sql);

    if (include_scripts && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->forwardEngineerScriptPosition() == "after_ddl")
          out_sql.append(user_script(*script));
      }
    }
    if (no_FK_for_inserts)
      out_sql.append(restore_server_vars());

    if (include_scripts && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->forwardEngineerScriptPosition() == "bottom_file")
          out_sql.append(user_script(*script));
      }
    }

    return out_sql;
  }
};

ssize_t DbMySQLImpl::makeSQLExportScript(GrtNamedObjectRef dbobject, grt::DictRef options,
                                         const grt::DictRef& createSQL, const grt::DictRef& dropSQL) {
  // now only catalog supported
  if (!db_mysql_CatalogRef::can_wrap(dbobject))
    return 1;

  db_mysql_CatalogRef catalog = db_mysql_CatalogRef::cast_from(dbobject);
  SQLExportComposer composer(options, createSQL, dropSQL);
  options.set("OutputScript", grt::StringRef(composer.get_export_sql(catalog)));
  return 0;
}

class SQLSyncComposer : public SQLComposer {
public:
  SQLSyncComposer(const grt::DictRef options) : SQLComposer(options) {
  }

  std::string get_sync_sql(const db_CatalogRef& cat, const grt::StringListRef& sql_list,
                           const grt::ListRef<GrtNamedObject>& obj_list) {
    std::string out_sql;
    std::list<int> views_indices;
    std::string view_placeholders;
    std::string views;
    std::string routines;
    std::string triggers;

    out_sql.append("-- MySQL Workbench Synchronization").append("\n");
    if (include_document_properties)
      out_sql.append(generateDocumentProperties(cat));
    out_sql.append("\n");

    if (include_scripts && cat.is_valid() && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->synchronizeScriptPosition() == "top_file")
          out_sql.append(user_script(*script));
      }
    }

    out_sql.append(set_server_vars());

    if (include_scripts && cat.is_valid() && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->synchronizeScriptPosition() == "before_ddl")
          out_sql.append(user_script(*script));
      }
    }

    for (size_t sz = sql_list.count(), i = 0; i < sz; i++) {
      GrtNamedObjectRef obj = obj_list.get(i);
      if (db_TriggerRef::can_wrap(obj)) {
        triggers.append(sql_list[i]).append(non_std_sql_delimiter).append("\n\n");
      } else if (db_RoutineRef::can_wrap(obj)) {
        routines.append(sql_list[i]);
      } else if (db_ViewRef::can_wrap(obj)) {
        std::string view_ddl(sql_list[i]);
        if (view_ddl.empty())
          continue;
        views_indices.push_back((int)i);
        db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(obj);
        view_placeholders.append(generate_view_placeholder(view));
      } else {
        out_sql.append(sql_list.get(i)).append(";\n\n");
      }
    }

    // views DDL
    // 2nd loop on views, 1st one creates view placeholders and filles alias_map
    for (std::list<int>::const_iterator e = views_indices.end(), it = views_indices.begin(); it != e; it++) {
      db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(obj_list.get(*it));
      std::string view_ddl(sql_list.get(*it));
      views.append("\n\nUSE `").append(view->owner()->name()).append("`;\n");
      views.append(generate_view_ddl(view, view_ddl));
    }

    out_sql.append(view_placeholders);
    out_sql.append(views);
    out_sql.append(routines);
    if (!triggers.empty()) {
      out_sql.append("\nDELIMITER ").append(non_std_sql_delimiter).append("\n\n");
      out_sql.append(triggers);
      out_sql.append("\nDELIMITER ;\n\n");
    }

    if (include_scripts && cat.is_valid() && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->synchronizeScriptPosition() == "after_ddl")
          out_sql.append(user_script(*script));
      }
    }

    out_sql.append(restore_server_vars());

    if (include_scripts && cat.is_valid() && cat->owner().is_valid()) {
      GRTLIST_FOREACH(db_Script, workbench_physical_ModelRef::cast_from(cat->owner())->scripts(), script) {
        if ((*script)->synchronizeScriptPosition() == "bottom_file")
          out_sql.append(user_script(*script));
      }
    }

    return out_sql;
  };
};

ssize_t DbMySQLImpl::makeSQLSyncScript(db_CatalogRef cat, grt::DictRef options, const grt::StringListRef& sql_list,
                                       const grt::ListRef<GrtNamedObject>& obj_list) {
  SQLSyncComposer composer(options);
  options.set("OutputScript", grt::StringRef(composer.get_sync_sql(cat, sql_list, obj_list)));
  return 0;
}

std::string DbMySQLImpl::makeAlterScript(GrtNamedObjectRef source, GrtNamedObjectRef target,
                                         const grt::DictRef& diff_options) {
  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = 3;
  grt::NormalizedComparer normalizer(grt::DictRef::cast_from(diff_options.get("DBSettings")));
  normalizer.init_omf(&omf);

  std::shared_ptr<DiffChange> diff = diff_make(source, target, &omf);
  if (!diff.get())
    return "";

  grt::DictRef options(true);
  grt::StringListRef alter_list(grt::Initialized);
  options.set("OutputContainer", alter_list);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("KeepOrder", grt::IntegerRef(1));
  grt::ListRef<GrtNamedObject> alter_object_list(true);
  options.set("OutputObjectContainer", alter_object_list);

  generateSQL(source, options, diff);

  db_CatalogRef cat;

  {
    GrtNamedObjectRef tmp(source);
    while (tmp.is_valid()) {
      if (db_CatalogRef::can_wrap(tmp)) {
        cat = db_CatalogRef::cast_from(source);
        break;
      }
      tmp = GrtNamedObjectRef::cast_from(tmp->owner());
    }
  }

  ssize_t res = makeSQLSyncScript(cat, options, alter_list, alter_object_list);
  if (res != 0)
    return "";

  grt::StringRef script = grt::StringRef::cast_from(options.get("OutputScript"));
  if (!script.is_valid())
    return "";

  return script;
}

std::string DbMySQLImpl::makeAlterScriptForObject(GrtNamedObjectRef source, GrtNamedObjectRef target,
                                                  GrtNamedObjectRef obj, const grt::DictRef& diff_options) {
  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = 5;

  DictRef options(true);
  DictRef result(true);

  options.set("UseFilteredLists", IntegerRef(0));
  grt::NormalizedComparer normalizer(grt::DictRef::cast_from(diff_options.get("DBSettings", getDefaultTraits())));
  normalizer.init_omf(&omf);
  bool case_sensitive = omf.case_sensitive;
  std::shared_ptr<DiffChange> diff = diff_make(source, target, &omf);

  std::string sql;
  std::string non_std_sql_delimiter = bec::GRTManager::get()->get_app_option_string("SqlDelimiter", "$$");

  if (diff.get()) {
    ActionGenerateSQL generator =
      ActionGenerateSQL(result, grt::ListRef<GrtNamedObject>(),
                        grt::DictRef::cast_from(diff_options.get("DBSettings", getDefaultTraits())));
    generator.set_put_if_exists(false);
    DiffSQLGeneratorBE(options, grt::DictRef::cast_from(diff_options.get("DBSettings", getDefaultTraits())), &generator)
      .process_diff_change(source, diff.get(), result);
    std::string objname = get_old_object_name_for_key(obj, omf.case_sensitive);
    ValueRef change = result.get(objname, StringRef(""));
    if (StringRef::can_wrap(change)) {
      sql = StringRef::cast_from(change);
      if (!sql.empty() && !db_RoutineRef::can_wrap(obj))
        sql.append(";\n");
    } else if (StringListRef::can_wrap(change)) {
      grt::StringListRef list = grt::StringListRef::cast_from(change);
      for (size_t listcount = list.count(), j = 0; j < listcount; j++)
        sql.append(list.get(j)).append(";\n");
    }

    if ((obj->name() != obj->oldName()) && !obj->oldName().empty()) {
      std::string objname = get_full_object_name_for_key(obj, case_sensitive != 0);
      ValueRef new_change = result.get(objname, StringRef(""));
      if (StringRef::can_wrap(new_change)) {
        std::string obj_ddl = StringRef::cast_from(new_change);
        sql.append(obj_ddl);
        if (!obj_ddl.empty())
          sql.append(";\n");
      } else if (StringListRef::can_wrap(new_change)) {
        grt::StringListRef list = grt::StringListRef::cast_from(new_change);
        for (size_t listcount = list.count(), j = 0; j < listcount; j++)
          sql.append(list.get(j)).append(";\n");
      }
    }

    if (db_TableRef::can_wrap(obj)) {
      db_mysql_TableRef table = db_mysql_TableRef::cast_from(obj);
      grt::ListRef<db_mysql_Trigger> triggers = table->triggers();
      if (db_CatalogRef::can_wrap(source)) {
        db_CatalogRef src_cat = db_CatalogRef::cast_from(source);
        for (size_t sz = src_cat->schemata().count(), i = 0; i < sz; i++) {
          if (strcmp(src_cat->schemata().get(i)->name().c_str(), table->owner()->name().c_str()) == 0) {
            db_SchemaRef schema = src_cat->schemata().get(i);
            for (size_t sz = schema->tables().count(), i = 0; i < sz; i++)
              if (strcmp(schema->tables().get(i)->name().c_str(), table->oldName().c_str()) == 0) {
                db_TableRef db_table = db_TableRef::cast_from(schema->tables().get(i));
                grt::ListRef<db_Trigger> db_triggers = db_table->triggers();
                for (size_t c = db_triggers.count(), i = 0; i < c; i++) {
                  bool trigger_found = false;
                  for (size_t c1 = triggers.count(), j = 0; j < c1; j++) {
                    if (db_triggers.get(i)->name() == triggers.get(j)->name()) {
                      trigger_found = true;
                      break;
                    }
                  }
                  if (!trigger_found) {
                    std::string trigger_code =
                      result.get_string(get_full_object_name_for_key(db_triggers.get(i), case_sensitive != 0), "");
                    if (!trigger_code.empty()) {
                      sql.append("USE `")
                        .append(db_table->owner()->name())
                        .append("`;\n")
                        .append("\nDELIMITER ")
                        .append(non_std_sql_delimiter)
                        .append("\n\n");
                      sql.append(trigger_code).append(non_std_sql_delimiter).append("\nDELIMITER ;\n");
                    }
                  }
                }
              }
          }
        }
      }

      for (size_t c = triggers.count(), i = 0; i < c; i++) {
        std::string trigger_code =
          result.get_string(get_full_object_name_for_key(triggers.get(i), case_sensitive != 0), "");
        if (!trigger_code.empty()) {
          std::string ddl = "DROP TRIGGER IF EXISTS !.!;\n\nDELIMITER " + non_std_sql_delimiter + "\n";
          sql += base::sqlstring(ddl.c_str(), 0) << table->owner()->name() << triggers.get(i)->name();
          sql += trigger_code + non_std_sql_delimiter + "\nDELIMITER ;\n";
        }
      }
    }

    if (!sql.empty())
      if (db_RoutineRef::can_wrap(obj) && ((obj->name() == obj->oldName()) || obj->oldName().empty())) {
        db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(obj);
        if (routine->routineType().empty())
          routine->routineType("procedure");
        std::string routine_sql("USE `");
        routine_sql.append(routine->owner()->name()).append("`;\n");
        routine_sql.append("DROP ")
          .append(routine->routineType().c_str())
          .append(" IF EXISTS `")
          .append(routine->name())
          .append("`;\n");
        sql = routine_sql + sql + std::string("\n");
      }
  }

  return sql;
}

// This function is used from scripts and HTML report generator.
std::string DbMySQLImpl::makeCreateScriptForObject(GrtNamedObjectRef object) {
  DictRef options(true);
  DictRef result(true);

  ValueRef parent;

  // TODO: check how this list is expected to be used
  // there should be a way to generate one table SQL only not sql for whole doc
  //  StringListRef list(grt::Initialized);
  //  list.insert(get_old_object_name_for_key(object));

  if (object.is_instance(db_Schema::static_class_name()))
    parent = object->owner();
  else if (object.is_instance(db_Table::static_class_name()))
    parent = object->owner()->owner();
  else if (object.is_instance(db_Trigger::static_class_name()))
    parent = object->owner()->owner()->owner();
  else if (object.is_instance(db_View::static_class_name()))
    parent = object->owner()->owner();
  else if (object.is_instance(db_Routine::static_class_name()))
    parent = object->owner()->owner();
  else if (object.is_instance(db_RoutineGroup::static_class_name()))
    parent = object->owner()->owner();
  else if (object.is_instance(db_User::static_class_name()))
    parent = object->owner();
  else if (object.is_instance(db_Role::static_class_name()))
    parent = object->owner();
  else
    return "";

  options.set("UseFilteredLists", IntegerRef(0));
  default_omf omf;
  grt::NormalizedComparer normalizer;
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> diff = diff_make(ValueRef(), parent, &omf, true); // do a diff without cloning the catalog

  std::string sql;

  if (diff.get()) {
    ActionGenerateSQL generator = ActionGenerateSQL(result, grt::ListRef<GrtNamedObject>(), getDefaultTraits());
    DiffSQLGeneratorBE(options, grt::DictRef::cast_from(options.get("DBSettings", getDefaultTraits())), &generator)
      .process_diff_change(ValueRef(), diff.get(), result);
    sql = result.get_string(get_full_object_name_for_key(object, omf.case_sensitive), "");
  }

  return sql;
}

db_mgmt_RdbmsRef DbMySQLImpl::initializeDBMSInfo() {
  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize(
    base::makePath(bec::GRTManager::get()->get_basedir(), "modules/data/mysql_rdbms_info.xml")));

  workbench_WorkbenchRef::cast_from(grt::GRT::get()->get("/wb"))->rdbmsMgmt()->rdbms().insert(rdbms);
  return rdbms;
}

grt::StringRef DbMySQLImpl::quoteIdentifier(grt::StringRef ident) {
  return grt::StringRef(base::sqlstring("!", 0) << *ident);
}

grt::StringRef DbMySQLImpl::fullyQualifiedObjectName(GrtNamedObjectRef object) {
  GrtNamedObjectRef owner = GrtNamedObjectRef::cast_from(object->owner());
  if (owner.is_valid()) {
    if (db_SchemaRef::can_wrap(owner))
      return grt::StringRef(base::sqlstring("!.!", 0) << owner->name() << *object->name());
  }
  return grt::StringRef(base::sqlstring("!", 0) << *object->name());
}

grt::ListRef<db_mysql_StorageEngine> DbMySQLImpl::getKnownEngines() {
  if (!_known_engines.is_valid())
    _known_engines = dbmysql::get_known_engines();
  return _known_engines;
}

// checks whether the 2nd version number is the same or newer than the 1st one
inline bool match_version(int ref_major, int ref_minor, int ref_revision, int major, int minor, int revision) {
  if (major > ref_major)
    return true;
  if (minor < ref_minor)
    return false;
  if (minor > ref_minor)
    return true;
  if (revision >= ref_revision)
    return true;
  return false;
}

grt::DictRef DbMySQLImpl::getTraitsForServerVersion(const int major, const int minor, const int revision) {
  grt::DictRef traits(true);

  traits.set("version", grt::StringRef(base::strfmt("%i.%i.%i", major, minor, revision < 0 ? 0 : revision)));

  if (match_version(5, 5, 3, major, minor, revision)) {
    traits.set("maxTableCommentLength", grt::IntegerRef(2048));
    traits.set("maxIndexCommentLength", grt::IntegerRef(1024));
    traits.set("maxColumnCommentLength", grt::IntegerRef(1024));
  } else {
    traits.set("maxTableCommentLength", grt::IntegerRef(60));
    traits.set("maxIndexCommentLength", grt::IntegerRef(0));
    traits.set("maxColumnCommentLength", grt::IntegerRef(255));
  }

  return traits;
}

grt::ListRef<db_UserDatatype> DbMySQLImpl::getDefaultUserDatatypes(db_mgmt_RdbmsRef rdbms) {
  static struct {
    const char* oid;
    const char* name;
    const char* sql_def;
  } type_init_data[] = {
    // These are type aliases, not UDTs
    { "com.mysql.rdbms.mysql.userdatatype.bool", "BOOL", "TINYINT(1)" },
    { "com.mysql.rdbms.mysql.userdatatype.boolean", "BOOLEAN", "TINYINT(1)" },
    { "com.mysql.rdbms.mysql.userdatatype.fixed", "FIXED", "DECIMAL(10,0)" },
    { "com.mysql.rdbms.mysql.userdatatype.float4", "FLOAT4", "FLOAT" },
    { "com.mysql.rdbms.mysql.userdatatype.float8", "FLOAT8", "DOUBLE" },
    { "com.mysql.rdbms.mysql.userdatatype.int1", "INT1", "TINYINT(4)" },
    { "com.mysql.rdbms.mysql.userdatatype.int2", "INT2", "SMALLINT(6)" },
    { "com.mysql.rdbms.mysql.userdatatype.int3", "INT3", "MEDIUMINT(9)" },
    { "com.mysql.rdbms.mysql.userdatatype.int4", "INT4", "INT(11)" },
    { "com.mysql.rdbms.mysql.userdatatype.int8", "INT8", "BIGINT(20)" },
    { "com.mysql.rdbms.mysql.userdatatype.integer", "INTEGER", "INT(11)" },
    { "com.mysql.rdbms.mysql.userdatatype.longvarbinary", "LONG VARBINARY", "MEDIUMBLOB" },
    { "com.mysql.rdbms.mysql.userdatatype.longvarchar", "LONG VARCHAR", "MEDIUMTEXT" },
    { "com.mysql.rdbms.mysql.userdatatype.long", "LONG", "MEDIUMTEXT" },
    { "com.mysql.rdbms.mysql.userdatatype.middleint", "MIDDLEINT", "MEDIUMINT(9)" },
    { "com.mysql.rdbms.mysql.userdatatype.numeric", "NUMERIC", "DECIMAL(10,0)" },
    { "com.mysql.rdbms.mysql.userdatatype.dec", "DEC", "DECIMAL(10,0)" },
    { "com.mysql.rdbms.mysql.userdatatype.character", "CHARACTER", "CHAR(1)" }
    // End type aliases
  };

  grt::ListRef<db_UserDatatype> list(true);

  for (size_t i = 0; i < sizeof(type_init_data) / sizeof(*type_init_data); i++) {
    std::string type = type_init_data[i].sql_def;
    std::string::size_type paren = type.find('(');
    if (paren != std::string::npos)
      type = type.substr(0, paren);

    db_SimpleDatatypeRef simpletype(
      parsers::MySQLParserServices::findDataType(rdbms->simpleDatatypes(), GrtVersionRef(), type));

    if (!simpletype.is_valid()) {
      continue;
    }
    db_UserDatatypeRef udata(grt::Initialized);

    udata->__set_id(type_init_data[i].oid);

    udata->name(type_init_data[i].name);
    udata->sqlDefinition(type_init_data[i].sql_def);
    udata->actualType(simpletype);

    list.insert(udata);
  }

  return list;
}

GRT_MODULE_ENTRY_POINT(DbMySQLImpl);

;
