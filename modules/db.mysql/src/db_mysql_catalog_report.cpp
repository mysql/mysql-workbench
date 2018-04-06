/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <cstring>
#include "grtdb/db_helpers.h"
#include "db_mysql_catalog_report.h"
#include "mtemplate/template.h"

/// XXX TODO
/// This should have a big parameter table and generate output parametrically
/// instead of implementing a new method for every new field. Maybe it wont
/// even need a table either.
/// --alfredo

std::string ActionGenerateReport::object_name(const GrtNamedObjectRef obj) const {
  std::string obj_name;
  obj_name += "`";
  if (!_omitSchemas) {
    obj_name += obj->owner()->name().c_str();
    obj_name += "`.`";
  }
  obj_name += obj->name().c_str();
  obj_name += "`";
  return obj_name;
}

std::string ActionGenerateReport::trigger_name(const GrtNamedObjectRef obj) const {
  std::string obj_name;
  obj_name += "`";
  if (!_omitSchemas) {
    obj_name += obj->owner()->owner()->name().c_str();
    obj_name += "`.`";
  }
  obj_name += obj->name().c_str();
  obj_name += "`";
  return obj_name;
}

static std::string get_index_columns(db_mysql_IndexRef index) {
  std::string col_list;
  for (size_t sz = index->columns().count(), i = 0; i < sz; i++) {
    if (i > 0)
      col_list += ", ";

    db_mysql_IndexColumnRef index_col = index->columns().get(i);
    col_list += index_col->referencedColumn()->name().c_str();

    if (index_col->descend())
      col_list += " (desc)";
  }

  return col_list;
}

static void get_fk_desc(db_mysql_ForeignKeyRef fk, std::string &col_list, std::string &ref_t, std::string &ref_col_list,
                        std::string &on_update, std::string &on_delete) {
  for (size_t sz = fk->columns().count(), i = 0; i < sz; i++) {
    if (i > 0)
      col_list += ", ";

    db_ColumnRef col = fk->columns().get(i);
    col_list += col->name().c_str();
  }

  db_mysql_TableRef ref_table = fk->referencedTable();
  ref_t.assign(ref_table->name().c_str());

  for (size_t sz = fk->referencedColumns().count(), i = 0; i < sz; i++) {
    if (i > 0)
      ref_col_list += ", ";

    db_ColumnRef col = fk->referencedColumns().get(i);
    ref_col_list += col->name().c_str();
  }

  if (strlen(fk->updateRule().c_str()))
    on_update.assign(fk->updateRule().c_str());
  else
    on_update.assign("none");

  if (strlen(fk->deleteRule().c_str()))
    on_delete.assign(fk->deleteRule().c_str());
  else
    on_delete.assign("none");
}

ActionGenerateReport::ActionGenerateReport(grt::StringRef template_filename)
  : fname(template_filename.c_str()), current_table_dictionary(nullptr),
    current_schema_dictionary(nullptr), has_attributes(false), has_partitioning(false) {
  dictionary = mtemplate::CreateMainDictionary();
}

ActionGenerateReport::~ActionGenerateReport() {
  delete dictionary;
}

std::string ActionGenerateReport::generate_output() {
  mtemplate::Template *tpl = mtemplate::GetTemplate(fname, mtemplate::STRIP_BLANK_LINES);
  mtemplate::TemplateOutputString output;
  tpl->expand(dictionary, &output);

  return output.get();
}

// create table
void ActionGenerateReport::create_table_props_begin(db_mysql_TableRef table) {
  current_table_dictionary = dictionary->addSectionDictionary(kbtr_CREATE_TABLE);
  current_table_dictionary->setValue(kbtr_CREATE_TABLE_NAME, object_name(table));

  has_attributes = false;
  has_partitioning = false;
}

void ActionGenerateReport::create_table_props_end(db_mysql_TableRef) {
  if (has_attributes) {
    current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_ATTRIBUTES_HEADER);
    current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_ATTRIBUTES_FOOTER);
  }
}

void ActionGenerateReport::create_table_columns_begin(db_mysql_TableRef) {
  current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_COLUMNS_HEADER);
}

void ActionGenerateReport::create_table_column(db_mysql_ColumnRef column) {
  mtemplate::DictionaryInterface *c2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_COLUMN);
  c2->setValue(kbtr_TABLE_COLUMN_NAME, (std::string)column->name());
  c2->setValue(kbtr_TABLE_COLUMN_TYPE, (std::string)(column->simpleType().is_valid() ? column->simpleType()->name()
                                                                                     : "<corrupted column type>"));
}

void ActionGenerateReport::create_table_columns_end(db_mysql_TableRef) {
  current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_COLUMNS_FOOTER);
}

void ActionGenerateReport::create_table_indexes_begin(db_mysql_TableRef table) {
  if (table->indices().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_INDEXES_HEADER);
}

void ActionGenerateReport::create_table_index(db_mysql_IndexRef index, bool gen_create_index) {
  mtemplate::DictionaryInterface *ix2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_INDEX);
  ix2->setValue(kbtr_TABLE_INDEX_NAME, (std::string)index->name());
  ix2->setValue(kbtr_TABLE_INDEX_COLUMNS, get_index_columns(index));
}

void ActionGenerateReport::create_table_indexes_end(db_mysql_TableRef table) {
  if (table->indices().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_INDEXES_FOOTER);
}

void ActionGenerateReport::create_table_fks_begin(db_mysql_TableRef table) {
  if (table->foreignKeys().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_FKS_HEADER);
}

void ActionGenerateReport::create_table_fk(db_mysql_ForeignKeyRef fk) {
  std::string col_list;
  std::string ref_table;
  std::string ref_col_list;
  std::string on_update;
  std::string on_delete;

  get_fk_desc(fk, col_list, ref_table, ref_col_list, on_update, on_delete);

  mtemplate::DictionaryInterface *f2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_FK);
  f2->setValue(kbtr_TABLE_FK_NAME, fk->name().c_str());
  f2->setValue(kbtr_TABLE_FK_COLUMNS, col_list);
  f2->setValue(kbtr_TABLE_FK_REF_TABLE, ref_table);
  f2->setValue(kbtr_TABLE_FK_REF_COLUMNS, ref_col_list);
  f2->setValue(kbtr_TABLE_FK_ON_UPDATE, on_update);
  f2->setValue(kbtr_TABLE_FK_ON_DELETE, on_delete);
}

void ActionGenerateReport::create_table_fks_end(db_mysql_TableRef table) {
  if (table->foreignKeys().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_CREATE_TABLE_FKS_FOOTER);
}

void ActionGenerateReport::create_table_engine(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_ENGINE);
  e2->setValue(kbtr_TABLE_ENGINE, (std::string)value);
}

void ActionGenerateReport::create_table_next_auto_inc(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_AUTOINC);
  e2->setValue(kbtr_TABLE_AUTOINC, (std::string)value);
}

void ActionGenerateReport::create_table_password(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_PASSWORD);
  e2->setValue(kbtr_TABLE_PASSWORD, (std::string)value);
}

void ActionGenerateReport::create_table_delay_key_write(grt::IntegerRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_DELAY_KEY_WRITE);
  e2->setValue(kbtr_TABLE_DELAY_KEY_WRITE, value.toString());
}

void ActionGenerateReport::create_table_charset(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_CHARSET);
  e->setValue(kbtr_TABLE_CHARSET, (std::string)value);
}

void ActionGenerateReport::create_table_collate(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_COLLATE);
  e2->setValue(kbtr_TABLE_COLLATE, (std::string)value);
}

void ActionGenerateReport::create_table_merge_union(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MERGE_UNION);
  e2->setValue(kbtr_TABLE_MERGE_UNION, (std::string)value);
}

void ActionGenerateReport::create_table_merge_insert(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MERGE_INSERT);
  e2->setValue(kbtr_TABLE_MERGE_INSERT, (std::string)value);
}

void ActionGenerateReport::create_table_pack_keys(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_PACK_KEYS);
  e2->setValue(kbtr_TABLE_PACK_KEYS, (std::string)value);
}

void ActionGenerateReport::create_table_checksum(grt::IntegerRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_CHECKSUM);
  e2->setValue(kbtr_TABLE_CHECKSUM, value.toString());
}

void ActionGenerateReport::create_table_row_format(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_ROW_FORMAT);
  e2->setValue(kbtr_TABLE_ROW_FORMAT, (std::string)value);
}

void ActionGenerateReport::create_table_key_block_size(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_KEY_BLOCK_SIZE);
  e2->setValue(kbtr_TABLE_KEY_BLOCK_SIZE, (std::string)value);
}

void ActionGenerateReport::create_table_avg_row_length(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_AVG_ROW_LENGTH);
  e2->setValue(kbtr_TABLE_AVG_ROW_LENGTH, (std::string)value);
}

void ActionGenerateReport::create_table_min_rows(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MIN_ROWS);
  e2->setValue(kbtr_TABLE_MIN_ROWS, (std::string)value);
}

void ActionGenerateReport::create_table_max_rows(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MAX_ROWS);
  e2->setValue(kbtr_TABLE_MAX_ROWS, (std::string)value);
}

void ActionGenerateReport::create_table_comment(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_COMMENT);
  e2->setValue(kbtr_TABLE_COMMENT, (std::string)value);
}

void ActionGenerateReport::create_table_data_dir(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_DATADIR);
  e2->setValue(kbtr_TABLE_DATADIR, (std::string)value);
}

void ActionGenerateReport::create_table_index_dir(grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_INDEXDIR);
  e2->setValue(kbtr_TABLE_INDEXDIR, (std::string)value);
}

// drop table
void ActionGenerateReport::drop_table(db_mysql_TableRef table) {
  current_table_dictionary = dictionary->addSectionDictionary(kbtr_DROP_TABLE);
  current_table_dictionary->setValue(kbtr_DROP_TABLE_NAME, object_name(table));
}

// alter table
void ActionGenerateReport::alter_table_props_begin(db_mysql_TableRef table) {
  current_table_dictionary = dictionary->addSectionDictionary(kbtr_ALTER_TABLE);
  current_table_dictionary->setValue(kbtr_ALTER_TABLE_NAME, object_name(table));

  has_attributes = false;
  has_partitioning = false;
}

void ActionGenerateReport::alter_table_name(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_NAME);
  e2->setValue(kbtr_NEW_TABLE_NAME, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_NAME, (std::string)table->name());
}

void ActionGenerateReport::alter_table_engine(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_ENGINE);
  e2->setValue(kbtr_NEW_TABLE_ENGINE, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_ENGINE, (std::string)table->tableEngine());
}

// currently auto_increment attribute is ignored during diff
void ActionGenerateReport::alter_table_next_auto_inc(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_AUTOINC);
  e2->setValue(kbtr_NEW_TABLE_AUTOINC, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_AUTOINC, (std::string)table->nextAutoInc());
}

void ActionGenerateReport::alter_table_password(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_PASSWORD);
  e2->setValue(kbtr_NEW_TABLE_PASSWORD, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_PASSWORD, (std::string)table->password());
}

void ActionGenerateReport::alter_table_delay_key_write(db_mysql_TableRef table, grt::IntegerRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_DELAY_KEY_WRITE);
  e2->setValue(kbtr_NEW_TABLE_DELAY_KEY_WRITE, value.toString());
  e2->setValue(kbtr_OLD_TABLE_DELAY_KEY_WRITE, table->delayKeyWrite().toString());
}

void ActionGenerateReport::alter_table_charset(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_CHARSET);
  e2->setValue(kbtr_NEW_TABLE_CHARSET, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_CHARSET, (std::string)table->defaultCharacterSetName());
}

void ActionGenerateReport::alter_table_collate(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_COLLATE);
  e2->setValue(kbtr_NEW_TABLE_COLLATE, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_COLLATE, (std::string)table->defaultCollationName());
}

void ActionGenerateReport::alter_table_merge_union(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MERGE_UNION);
  e2->setValue(kbtr_NEW_TABLE_MERGE_UNION, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_MERGE_UNION, (std::string)table->mergeUnion());
}

void ActionGenerateReport::alter_table_merge_insert(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MERGE_INSERT);
  e2->setValue(kbtr_NEW_TABLE_MERGE_INSERT, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_MERGE_INSERT, (std::string)table->mergeInsert());
}

void ActionGenerateReport::alter_table_pack_keys(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_PACK_KEYS);
  e2->setValue(kbtr_NEW_TABLE_PACK_KEYS, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_PACK_KEYS, (std::string)table->packKeys());
}

void ActionGenerateReport::alter_table_checksum(db_mysql_TableRef table, grt::IntegerRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_CHECKSUM);
  e2->setValue(kbtr_NEW_TABLE_CHECKSUM, (std::string)value.toString());
  e2->setValue(kbtr_OLD_TABLE_CHECKSUM, (std::string)table->checksum().toString());
}

void ActionGenerateReport::alter_table_row_format(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_ROW_FORMAT);
  e2->setValue(kbtr_NEW_TABLE_ROW_FORMAT, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_ROW_FORMAT, (std::string)table->rowFormat());
}

void ActionGenerateReport::alter_table_key_block_size(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_KEY_BLOCK_SIZE);
  e2->setValue(kbtr_NEW_TABLE_KEY_BLOCK_SIZE, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_KEY_BLOCK_SIZE, (std::string)table->keyBlockSize());
}

void ActionGenerateReport::alter_table_comment(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_COMMENT);
  e2->setValue(kbtr_NEW_TABLE_COMMENT, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_COMMENT, (std::string)table->comment());
}

void ActionGenerateReport::alter_table_avg_row_length(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_AVG_ROW_LENGTH);
  e2->setValue(kbtr_NEW_TABLE_AVG_ROW_LENGTH, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_AVG_ROW_LENGTH, (std::string)table->avgRowLength());
}

void ActionGenerateReport::alter_table_min_rows(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MIN_ROWS);
  e2->setValue(kbtr_NEW_TABLE_MIN_ROWS, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_MIN_ROWS, (std::string)table->minRows());
}

void ActionGenerateReport::alter_table_max_rows(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_MAX_ROWS);
  e2->setValue(kbtr_NEW_TABLE_MAX_ROWS, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_MAX_ROWS, (std::string)table->maxRows());
}

void ActionGenerateReport::alter_table_connection_string(db_mysql_TableRef table, grt::StringRef value) {
  has_attributes = true;

  mtemplate::DictionaryInterface *e2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_ATTR_COMMENT);
  e2->setValue(kbtr_NEW_TABLE_COMMENT, (std::string)value);
  e2->setValue(kbtr_OLD_TABLE_COMMENT, (std::string)table->comment());
}

void ActionGenerateReport::alter_table_generate_partitioning(db_mysql_TableRef table, const std::string &part_type,
                                                             const std::string &part_expr, int part_count,
                                                             const std::string &subpart_type,
                                                             const std::string &subpart_expr,
                                                             grt::ListRef<db_mysql_PartitionDefinition> part_defs) {
  bool is_new = (strlen(table->partitionType().c_str()) == 0);

  if (is_new)
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_ADDED);
  else
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);

  has_partitioning = true;
}

void ActionGenerateReport::alter_table_drop_partitioning(db_mysql_TableRef table) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_REMOVED);
  has_partitioning = true;
}

void ActionGenerateReport::alter_table_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning = true;
}

void ActionGenerateReport::alter_table_drop_partition(const std::string &part_name) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning = true;
}

void ActionGenerateReport::alter_table_reorganize_partition(db_mysql_PartitionDefinitionRef old_part,
                                                            db_mysql_PartitionDefinitionRef new_part, bool is_range) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning = true;
}

void ActionGenerateReport::alter_table_partition_count(db_mysql_TableRef, grt::IntegerRef) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning = true;
}

void ActionGenerateReport::alter_table_partition_definitions(db_mysql_TableRef, grt::StringRef) {
  has_partitioning = true;
}

void ActionGenerateReport::alter_table_props_end(db_mysql_TableRef) {
  if (has_attributes) {
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_ATTRIBUTES_HEADER);
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_ATTRIBUTES_FOOTER);
  }
  if (has_partitioning) {
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_HEADER);
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_PART_FOOTER);
  }
}

void ActionGenerateReport::alter_table_columns_begin(db_mysql_TableRef table) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_COLUMNS_HEADER);
}

void ActionGenerateReport::alter_table_add_column(db_mysql_TableRef, std::map<std::string, std::string>,
                                                  db_mysql_ColumnRef column, db_mysql_ColumnRef after) {
  mtemplate::DictionaryInterface *c2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_COLUMN_ADDED);
  c2->setValue(kbtr_TABLE_COLUMN_NAME, (std::string)column->name());
  c2->setValue(kbtr_TABLE_COLUMN_TYPE, (std::string)column->formattedType());
}

void ActionGenerateReport::alter_table_drop_column(db_mysql_TableRef, db_mysql_ColumnRef column) {
  mtemplate::DictionaryInterface *c2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_COLUMN_REMOVED);
  c2->setValue(kbtr_TABLE_COLUMN_NAME, (std::string)column->name());
}

void ActionGenerateReport::alter_table_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col,
                                                     db_mysql_ColumnRef mod_col, db_mysql_ColumnRef after,
                                                     bool modified,
                                                     std::map<std::string, std::string> column_rename_map) {
  mtemplate::DictionaryInterface *c2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_COLUMN_MODIFIED);
  c2->setValue(kbtr_TABLE_COLUMN_NAME, (std::string)org_col->name());
}

void ActionGenerateReport::alter_table_columns_end(db_mysql_TableRef) {
  current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_COLUMNS_FOOTER);
}

void ActionGenerateReport::alter_table_indexes_begin(db_mysql_TableRef table) {
  if (table->indices().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_INDEXES_HEADER);
}

void ActionGenerateReport::alter_table_add_index(db_mysql_IndexRef index) {
  mtemplate::DictionaryInterface *ix2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_INDEX_ADDED);
  ix2->setValue(kbtr_TABLE_INDEX_NAME, (std::string)index->name());
  ix2->setValue(kbtr_TABLE_INDEX_COLUMNS, get_index_columns(index));
}

void ActionGenerateReport::alter_table_drop_index(db_mysql_IndexRef index) {
  mtemplate::DictionaryInterface *ix2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_INDEX_REMOVED);
  ix2->setValue(kbtr_TABLE_INDEX_NAME, (std::string)index->name());
}

void ActionGenerateReport::alter_table_indexes_end(db_mysql_TableRef table) {
  if (table->indices().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_INDEXES_FOOTER);
}

void ActionGenerateReport::alter_table_change_index(db_mysql_IndexRef orgIndex, db_mysql_IndexRef newIndex) {
  auto catalog = db_CatalogRef::cast_from(orgIndex->owner()->owner()->owner());

  if (!bec::is_supported_mysql_version_at_least(catalog->version(), 8, 0, 0)) {
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
    if (orgColumns[i]->referencedColumn()->name() != newColumns[i]->referencedColumn()->name()) {
      alter_table_drop_index(newIndex);
      alter_table_add_index(newIndex);
      return;
    }
  }

  auto properties = { "algorithm", "keyBlockSize", "lockOption", "withParser", "visible" };

  std::vector<std::string> diffProps;
  for (const auto &it : properties) {
    if (orgIndex->get_member(it) != newIndex->get_member(it))
      diffProps.push_back(it);
  }

  if (diffProps.size() > 1 || diffProps[0] != "visible") {
    alter_table_drop_index(newIndex);
    alter_table_add_index(newIndex);
    return;
  }

  mtemplate::DictionaryInterface *ix2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_INDEX_MODIFIED);
  ix2->setValue(kbtr_TABLE_INDEX_NAME, (std::string)newIndex->name());
}

void ActionGenerateReport::alter_table_fks_begin(db_mysql_TableRef table) {
  if (table->foreignKeys().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_FKS_HEADER);
}

void ActionGenerateReport::alter_table_add_fk(db_mysql_ForeignKeyRef fk) {
  std::string col_list;
  std::string ref_table;
  std::string ref_col_list;
  std::string on_update;
  std::string on_delete;

  get_fk_desc(fk, col_list, ref_table, ref_col_list, on_update, on_delete);

  mtemplate::DictionaryInterface *f2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_FK_ADDED);
  f2->setValue(kbtr_TABLE_FK_NAME, (std::string)fk->name());
  f2->setValue(kbtr_TABLE_FK_COLUMNS, col_list);
  f2->setValue(kbtr_TABLE_FK_REF_TABLE, ref_table);
  f2->setValue(kbtr_TABLE_FK_REF_COLUMNS, ref_col_list);
  f2->setValue(kbtr_TABLE_FK_ON_UPDATE, on_update);
  f2->setValue(kbtr_TABLE_FK_ON_DELETE, on_delete);
}

void ActionGenerateReport::alter_table_drop_fk(db_mysql_ForeignKeyRef fk) {
  mtemplate::DictionaryInterface *f2 = current_table_dictionary->addSectionDictionary(kbtr_TABLE_FK_REMOVED);
  f2->setValue(kbtr_TABLE_FK_NAME, (std::string)fk->name());
}

void ActionGenerateReport::alter_table_fks_end(db_mysql_TableRef table) {
  if (table->foreignKeys().count() > 0)
    current_table_dictionary->addSectionDictionary(kbtr_ALTER_TABLE_FKS_FOOTER);
}

// triggers create/drop
void ActionGenerateReport::create_trigger(db_mysql_TriggerRef trigger, bool for_alter) {
  dictionary->addSectionDictionary(kbtr_CREATE_TRIGGER)->setValue(kbtr_CREATE_TRIGGER_NAME, trigger_name(trigger));
}

void ActionGenerateReport::drop_trigger(db_mysql_TriggerRef trigger, bool for_alter) {
  dictionary->addSectionDictionary(kbtr_DROP_TRIGGER)->setValue(kbtr_DROP_TRIGGER_NAME, trigger_name(trigger));
}

// views create/drop
void ActionGenerateReport::create_view(db_mysql_ViewRef view) {
  dictionary->addSectionDictionary(kbtr_CREATE_VIEW)->setValue(kbtr_CREATE_VIEW_NAME, object_name(view));
}

void ActionGenerateReport::drop_view(db_mysql_ViewRef view) {
  dictionary->addSectionDictionary(kbtr_DROP_VIEW)->setValue(kbtr_DROP_VIEW_NAME, object_name(view));
}

// routines create/drop
void ActionGenerateReport::create_routine(db_mysql_RoutineRef routine, bool for_alter) {
  dictionary->addSectionDictionary(kbtr_CREATE_ROUTINE)->setValue(kbtr_CREATE_ROUTINE_NAME, object_name(routine));
}

void ActionGenerateReport::drop_routine(db_mysql_RoutineRef routine, bool for_alter) {
  dictionary->addSectionDictionary(kbtr_DROP_ROUTINE)->setValue(kbtr_DROP_ROUTINE_NAME, object_name(routine));
}

// users create/drop
void ActionGenerateReport::create_user(db_UserRef user) {
  dictionary->addSectionDictionary(kbtr_CREATE_USER)->setValue(kbtr_CREATE_USER_NAME, object_name(user));
}

void ActionGenerateReport::drop_user(db_UserRef user) {
  dictionary->addSectionDictionary(kbtr_DROP_USER)->setValue(kbtr_DROP_USER_NAME, object_name(user));
}

// schema create/drop
void ActionGenerateReport::create_schema(db_mysql_SchemaRef schema) {
  dictionary->addSectionDictionary(kbtr_CREATE_SCHEMA)->setValue(kbtr_CREATE_SCHEMA_NAME, object_name(schema));
}

void ActionGenerateReport::drop_schema(db_mysql_SchemaRef schema) {
  dictionary->addSectionDictionary(kbtr_DROP_SCHEMA)->setValue(kbtr_DROP_SCHEMA_NAME, object_name(schema));
}

// alter schema
void ActionGenerateReport::alter_schema_props_begin(db_mysql_SchemaRef) {
  current_schema_dictionary = NULL;
}

void ActionGenerateReport::alter_schema_name(db_mysql_SchemaRef schema, grt::StringRef value) {
  if (current_schema_dictionary == NULL) {
    current_schema_dictionary = dictionary->addSectionDictionary(kbtr_ALTER_SCHEMA);
    current_schema_dictionary->setValue(kbtr_ALTER_SCHEMA_NAME, object_name(schema));
  }

  mtemplate::DictionaryInterface *c2 = current_schema_dictionary->addSectionDictionary(kbtr_ALTER_SCHEMA_NAME);
  c2->setValue(kbtr_OLD_SCHEMA_NAME, (std::string)schema->name());
  c2->setValue(kbtr_NEW_SCHEMA_NAME, (std::string)value);
}

void ActionGenerateReport::alter_schema_default_charset(db_mysql_SchemaRef schema, grt::StringRef value) {
  if (current_schema_dictionary == NULL) {
    current_schema_dictionary = dictionary->addSectionDictionary(kbtr_ALTER_SCHEMA);
    current_schema_dictionary->setValue(kbtr_ALTER_SCHEMA_NAME, object_name(schema));
  }

  // schema_altered= true;
  mtemplate::DictionaryInterface *c2 = current_schema_dictionary->addSectionDictionary(kbtr_ALTER_SCHEMA_CHARSET);
  c2->setValue(kbtr_OLD_SCHEMA_CHARSET, (std::string)schema->defaultCharacterSetName());
  c2->setValue(kbtr_NEW_SCHEMA_CHARSET, (std::string)value);
}

void ActionGenerateReport::alter_schema_default_collate(db_mysql_SchemaRef schema, grt::StringRef value) {
  if (current_schema_dictionary == NULL) {
    current_schema_dictionary = dictionary->addSectionDictionary(kbtr_ALTER_SCHEMA);
    current_schema_dictionary->setValue(kbtr_ALTER_SCHEMA_NAME, object_name(schema));
  }

  // schema_altered= true;
  mtemplate::DictionaryInterface *c2 = current_schema_dictionary->addSectionDictionary(kbtr_ALTER_SCHEMA_CHARSET);
  c2->setValue(kbtr_OLD_SCHEMA_COLLATE, (std::string)schema->defaultCollationName());
  c2->setValue(kbtr_NEW_SCHEMA_COLLATE, (std::string)value);
}

void ActionGenerateReport::alter_schema_props_end(db_mysql_SchemaRef schema) {
}
