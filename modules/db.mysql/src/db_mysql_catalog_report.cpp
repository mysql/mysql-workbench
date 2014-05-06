/*
* Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <cstring>
#include "db_mysql_catalog_report.h"

///XXX TODO 
/// This should have a big parameter table and generate output parametrically
/// instead of implementing a new method for every new field. Maybe it wont
/// even need a table either.
/// --alfredo

std::string ActionGenerateReport::object_name(const GrtNamedObjectRef obj)const
{
  std::string obj_name;
  obj_name += "`";
  if (!_use_short_names)
  {
    obj_name += obj->owner()->name().c_str();
    obj_name += "`.`";
  }
  obj_name += obj->name().c_str();
  obj_name += "`";
  return obj_name;
}

std::string ActionGenerateReport::trigger_name(const GrtNamedObjectRef obj)const
{
  std::string obj_name;
  obj_name += "`";
  if (!_use_short_names)
  {
    obj_name += obj->owner()->owner()->name().c_str();
    obj_name += "`.`";
  }
  obj_name += obj->name().c_str();
  obj_name += "`";
  return obj_name;
}

static std::string get_index_columns(db_mysql_IndexRef index)
{
  std::string col_list;
  for(size_t sz = index->columns().count(), i = 0; i < sz; i++)
  {
    if(i > 0)
      col_list += ", ";

    db_mysql_IndexColumnRef index_col= index->columns().get(i);
    col_list += index_col->referencedColumn()->name().c_str();

    if(index_col->descend())
      col_list += " (desc)";
  }

  return col_list;
}

static void get_fk_desc(db_mysql_ForeignKeyRef fk, 
                        std::string& col_list, std::string& ref_t, 
                        std::string& ref_col_list, std::string& on_update, 
                        std::string& on_delete)
{
  for (size_t sz = fk->columns().count(), i = 0; i < sz; i++)
  {
    if(i > 0)
      col_list += ", ";

    db_ColumnRef col= fk->columns().get(i);
    col_list += col->name().c_str();
  }

  db_mysql_TableRef ref_table= fk->referencedTable();
  ref_t.assign(ref_table->name().c_str());

  for (size_t sz = fk->referencedColumns().count(), i = 0; i < sz; i++)
  {
    if(i > 0)
      ref_col_list += ", ";

    db_ColumnRef col= fk->referencedColumns().get(i);
    ref_col_list += col->name().c_str();
  }

  if(strlen(fk->updateRule().c_str()))
    on_update.assign(fk->updateRule().c_str());
  else
    on_update.assign("none");

  if(strlen(fk->deleteRule().c_str()))
    on_delete.assign(fk->deleteRule().c_str());
  else
    on_delete.assign("none");
}

ActionGenerateReport::ActionGenerateReport(grt::StringRef template_filename)
  : fname(template_filename.c_str()), dict("catalog diff report")
{
}

ActionGenerateReport::~ActionGenerateReport()
{}

std::string ActionGenerateReport::generate_output()
{
  Template* tpl = Template::GetTemplate(fname.c_str(), ctemplate::STRIP_BLANK_LINES);
  if (!tpl)
    throw std::logic_error("Unable to locate template file '"+fname+"'");
  std::string output;
  tpl->Expand(&output, &dict);
  return output;
}

// create table
void ActionGenerateReport::create_table_props_begin(db_mysql_TableRef table)
{
  curr_table= dict.AddSectionDictionary(kbtr_CREATE_TABLE);
  curr_table->SetValue(kbtr_CREATE_TABLE_NAME, object_name(table).c_str());
  has_attributes= false;
  has_partitioning= false;
}

void ActionGenerateReport::create_table_props_end(db_mysql_TableRef)
{
  if(has_attributes)
  {
    curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_ATTRIBUTES_HEADER);
    curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_ATTRIBUTES_FOOTER);
  }
}

void ActionGenerateReport::create_table_columns_begin(db_mysql_TableRef)
{
  curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_COLUMNS_HEADER);
}

void ActionGenerateReport::create_table_column(db_mysql_ColumnRef column)
{
  TemplateDictionary *c= curr_table->AddSectionDictionary(kbtr_TABLE_COLUMN);
  c->SetValue(kbtr_TABLE_COLUMN_NAME, column->name().c_str());
  c->SetValue(kbtr_TABLE_COLUMN_TYPE, column->simpleType().is_valid() ? column->simpleType()->name().c_str() : "<corrupted column type>");
}

void ActionGenerateReport::create_table_columns_end(db_mysql_TableRef)
{
  curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_COLUMNS_FOOTER);
}

void ActionGenerateReport::create_table_indexes_begin(db_mysql_TableRef table)
{
  if(table->indices().count() > 0)
    curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_INDEXES_HEADER);
}

void ActionGenerateReport::create_table_index(db_mysql_IndexRef index, bool gen_create_index)
{
  TemplateDictionary *ix= curr_table->AddSectionDictionary(kbtr_TABLE_INDEX);
  ix->SetValue(kbtr_TABLE_INDEX_NAME, index->name().c_str());
  ix->SetValue(kbtr_TABLE_INDEX_COLUMNS, get_index_columns(index));
}

void ActionGenerateReport::create_table_indexes_end(db_mysql_TableRef table)
{
  if(table->indices().count() > 0)
    curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_INDEXES_FOOTER);
}


void ActionGenerateReport::create_table_fks_begin(db_mysql_TableRef table)
{
  if(table->foreignKeys().count() > 0)
    curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_FKS_HEADER);
}

void ActionGenerateReport::create_table_fk(db_mysql_ForeignKeyRef fk)
{
  TemplateDictionary *f= curr_table->AddSectionDictionary(kbtr_TABLE_FK);
  f->SetValue(kbtr_TABLE_FK_NAME, fk->name().c_str());

  std::string col_list;
  std::string ref_table;
  std::string ref_col_list;
  std::string on_update;
  std::string on_delete;

  get_fk_desc(fk, col_list, ref_table, ref_col_list, on_update, on_delete);

  f->SetValue(kbtr_TABLE_FK_COLUMNS, col_list);
  f->SetValue(kbtr_TABLE_FK_REF_TABLE, ref_table);
  f->SetValue(kbtr_TABLE_FK_REF_COLUMNS, ref_col_list);
  f->SetValue(kbtr_TABLE_FK_ON_UPDATE, on_update);
  f->SetValue(kbtr_TABLE_FK_ON_DELETE, on_delete);
}

void ActionGenerateReport::create_table_fks_end(db_mysql_TableRef table)
{
  if(table->foreignKeys().count() > 0)
    curr_table->AddSectionDictionary(kbtr_CREATE_TABLE_FKS_FOOTER);
}


void ActionGenerateReport::create_table_engine(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_ENGINE);
  e->SetValue(kbtr_TABLE_ENGINE, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_next_auto_inc(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_AUTOINC);
  e->SetValue(kbtr_TABLE_AUTOINC, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_password(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_PASSWORD);
  e->SetValue(kbtr_TABLE_PASSWORD, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_delay_key_write(grt::IntegerRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_DELAY_KEY_WRITE);
  e->SetValue(kbtr_TABLE_DELAY_KEY_WRITE, value.repr().c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_charset(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_CHARSET);
  e->SetValue(kbtr_TABLE_CHARSET, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_collate(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_COLLATE);
  e->SetValue(kbtr_TABLE_COLLATE, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_merge_union(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MERGE_UNION);
  e->SetValue(kbtr_TABLE_MERGE_UNION, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_merge_insert(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MERGE_INSERT);
  e->SetValue(kbtr_TABLE_MERGE_INSERT, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_pack_keys(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_PACK_KEYS);
  e->SetValue(kbtr_TABLE_PACK_KEYS, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_checksum(grt::IntegerRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_CHECKSUM);
  e->SetValue(kbtr_TABLE_CHECKSUM, value.repr().c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_row_format(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_ROW_FORMAT);
  e->SetValue(kbtr_TABLE_ROW_FORMAT, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_key_block_size(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_KEY_BLOCK_SIZE);
  e->SetValue(kbtr_TABLE_KEY_BLOCK_SIZE, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_avg_row_length(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_AVG_ROW_LENGTH);
  e->SetValue(kbtr_TABLE_AVG_ROW_LENGTH, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_min_rows(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MIN_ROWS);
  e->SetValue(kbtr_TABLE_MIN_ROWS, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_max_rows(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MAX_ROWS);
  e->SetValue(kbtr_TABLE_MAX_ROWS, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_comment(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_COMMENT);
  e->SetValue(kbtr_TABLE_COMMENT, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_data_dir(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_DATADIR);
  e->SetValue(kbtr_TABLE_DATADIR, value.c_str());
  has_attributes= true;
}

void ActionGenerateReport::create_table_index_dir(grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_INDEXDIR);
  e->SetValue(kbtr_TABLE_INDEXDIR, value.c_str());
  has_attributes= true;
}

// drop table
void ActionGenerateReport::drop_table(db_mysql_TableRef table)
{
  curr_table= dict.AddSectionDictionary(kbtr_DROP_TABLE);
  curr_table->SetValue(kbtr_DROP_TABLE_NAME, object_name(table).c_str());
}

// alter table
void ActionGenerateReport::alter_table_props_begin(db_mysql_TableRef table)
{
  curr_table= dict.AddSectionDictionary(kbtr_ALTER_TABLE);
  curr_table->SetValue(kbtr_ALTER_TABLE_NAME, object_name(table).c_str());
  has_attributes= false;
  has_partitioning= false;
}

void ActionGenerateReport::alter_table_name(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_NAME);
  e->SetValue(kbtr_NEW_TABLE_NAME, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_NAME, table->name().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_engine(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_ENGINE);
  e->SetValue(kbtr_NEW_TABLE_ENGINE, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_ENGINE, table->tableEngine().c_str());
  has_attributes= true;
}

// currently auto_increment attribute is ignored during diff
void ActionGenerateReport::alter_table_next_auto_inc(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_AUTOINC);
  e->SetValue(kbtr_NEW_TABLE_AUTOINC, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_AUTOINC, table->nextAutoInc().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_password(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_PASSWORD);
  e->SetValue(kbtr_NEW_TABLE_PASSWORD, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_PASSWORD, table->password().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_delay_key_write(db_mysql_TableRef table, grt::IntegerRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_DELAY_KEY_WRITE);
  e->SetValue(kbtr_NEW_TABLE_DELAY_KEY_WRITE, value.repr().c_str());
  e->SetValue(kbtr_OLD_TABLE_DELAY_KEY_WRITE, table->delayKeyWrite().repr().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_charset(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_CHARSET);
  e->SetValue(kbtr_NEW_TABLE_CHARSET, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_CHARSET, table->defaultCharacterSetName().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_collate(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_COLLATE);
  e->SetValue(kbtr_NEW_TABLE_COLLATE, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_COLLATE, table->defaultCollationName().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_merge_union(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MERGE_UNION);
  e->SetValue(kbtr_NEW_TABLE_MERGE_UNION, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_MERGE_UNION, table->mergeUnion().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_merge_insert(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MERGE_INSERT);
  e->SetValue(kbtr_NEW_TABLE_MERGE_INSERT, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_MERGE_INSERT, table->mergeInsert().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_pack_keys(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_PACK_KEYS);
  e->SetValue(kbtr_NEW_TABLE_PACK_KEYS, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_PACK_KEYS, table->packKeys().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_checksum(db_mysql_TableRef table, grt::IntegerRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_CHECKSUM);
  e->SetValue(kbtr_NEW_TABLE_CHECKSUM, value.repr().c_str());
  e->SetValue(kbtr_OLD_TABLE_CHECKSUM, table->checksum().repr().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_row_format(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_ROW_FORMAT);
  e->SetValue(kbtr_NEW_TABLE_ROW_FORMAT, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_ROW_FORMAT, table->rowFormat().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_key_block_size(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_KEY_BLOCK_SIZE);
  e->SetValue(kbtr_NEW_TABLE_KEY_BLOCK_SIZE, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_KEY_BLOCK_SIZE, table->keyBlockSize().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_comment(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_COMMENT);
  e->SetValue(kbtr_NEW_TABLE_COMMENT, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_COMMENT, table->comment().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_avg_row_length(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_AVG_ROW_LENGTH);
  e->SetValue(kbtr_NEW_TABLE_AVG_ROW_LENGTH, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_AVG_ROW_LENGTH, table->avgRowLength().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_min_rows(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MIN_ROWS);
  e->SetValue(kbtr_NEW_TABLE_MIN_ROWS, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_MIN_ROWS, table->minRows().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_max_rows(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_MAX_ROWS);
  e->SetValue(kbtr_NEW_TABLE_MAX_ROWS, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_MAX_ROWS, table->maxRows().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_connection_string(db_mysql_TableRef table, grt::StringRef value)
{
  TemplateDictionary *e= curr_table->AddSectionDictionary(kbtr_TABLE_ATTR_COMMENT);
  e->SetValue(kbtr_NEW_TABLE_COMMENT, value.c_str());
  e->SetValue(kbtr_OLD_TABLE_COMMENT, table->comment().c_str());
  has_attributes= true;
}

void ActionGenerateReport::alter_table_generate_partitioning(db_mysql_TableRef table, 
                                               const std::string& part_type,
                                               const std::string& part_expr, 
                                               int part_count,
                                               const std::string& subpart_type, 
                                               const std::string& subpart_expr,
                                               grt::ListRef<db_mysql_PartitionDefinition> part_defs)
{
  bool is_new= (strlen(table->partitionType().c_str()) == 0);

  if(is_new)
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_ADDED);
  else
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);

  has_partitioning= true;
}

void ActionGenerateReport::alter_table_drop_partitioning(db_mysql_TableRef table)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_REMOVED);
  has_partitioning= true;
}

void ActionGenerateReport::alter_table_add_partition(db_mysql_PartitionDefinitionRef part, bool is_range)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning= true;
}

void ActionGenerateReport::alter_table_drop_partition(const std::string& part_name)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning= true;
}

void ActionGenerateReport::alter_table_reorganize_partition(
                                          db_mysql_PartitionDefinitionRef old_part,
                                          db_mysql_PartitionDefinitionRef new_part,
                                          bool is_range)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning= true;
}

void ActionGenerateReport::alter_table_partition_count(db_mysql_TableRef, grt::IntegerRef)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_MODIFIED);
  has_partitioning= true;
}

void ActionGenerateReport::alter_table_partition_definitions(db_mysql_TableRef, grt::StringRef)
{
  has_partitioning= true;
}

void ActionGenerateReport::alter_table_props_end(db_mysql_TableRef)
{
  if(has_attributes)
  {
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_ATTRIBUTES_HEADER);
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_ATTRIBUTES_FOOTER);
  }
  if(has_partitioning)
  {
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_HEADER);
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_PART_FOOTER);
  }
}

void ActionGenerateReport::alter_table_columns_begin(db_mysql_TableRef table)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_COLUMNS_HEADER);
}

void ActionGenerateReport::alter_table_add_column(db_mysql_TableRef, std::map<std::string, std::string>, 
                                    db_mysql_ColumnRef column, db_mysql_ColumnRef after)
{

  TemplateDictionary *c= curr_table->AddSectionDictionary(kbtr_TABLE_COLUMN_ADDED);
  c->SetValue(kbtr_TABLE_COLUMN_NAME, column->name().c_str());
  c->SetValue(kbtr_TABLE_COLUMN_TYPE, column->formattedType().c_str());
}

void ActionGenerateReport::alter_table_drop_column(db_mysql_TableRef, db_mysql_ColumnRef column)
{
  TemplateDictionary *c= curr_table->AddSectionDictionary(kbtr_TABLE_COLUMN_REMOVED);
  c->SetValue(kbtr_TABLE_COLUMN_NAME, column->name().c_str());
}

void ActionGenerateReport::alter_table_change_column(db_mysql_TableRef table, db_mysql_ColumnRef org_col, 
                                       db_mysql_ColumnRef mod_col, db_mysql_ColumnRef after,
                                       bool modified, 
                                       std::map<std::string, std::string> column_rename_map)
{
  TemplateDictionary *c= curr_table->AddSectionDictionary(kbtr_TABLE_COLUMN_MODIFIED);
  c->SetValue(kbtr_TABLE_COLUMN_NAME, org_col->name().c_str());
}

void ActionGenerateReport::alter_table_columns_end(db_mysql_TableRef)
{
  curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_COLUMNS_FOOTER);
}

void ActionGenerateReport::alter_table_indexes_begin(db_mysql_TableRef table)
{
  if(table->indices().count() > 0)
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_INDEXES_HEADER);    
}

void ActionGenerateReport::alter_table_add_index(db_mysql_IndexRef index)
{
  TemplateDictionary *ix= curr_table->AddSectionDictionary(kbtr_TABLE_INDEX_ADDED);
  ix->SetValue(kbtr_TABLE_INDEX_NAME, index->name().c_str());
  ix->SetValue(kbtr_TABLE_INDEX_COLUMNS, get_index_columns(index));
}

void ActionGenerateReport::alter_table_drop_index(db_mysql_IndexRef index)
{
  TemplateDictionary *ix= curr_table->AddSectionDictionary(kbtr_TABLE_INDEX_REMOVED);
  ix->SetValue(kbtr_TABLE_INDEX_NAME, index->name().c_str());
}

void ActionGenerateReport::alter_table_indexes_end(db_mysql_TableRef table)
{
  if(table->indices().count() > 0)
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_INDEXES_FOOTER);    
}

void ActionGenerateReport::alter_table_fks_begin(db_mysql_TableRef table)
{
  if(table->foreignKeys().count() > 0)
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_FKS_HEADER);    
}

void ActionGenerateReport::alter_table_add_fk(db_mysql_ForeignKeyRef fk)
{
  TemplateDictionary *f= curr_table->AddSectionDictionary(kbtr_TABLE_FK_ADDED);
  f->SetValue(kbtr_TABLE_FK_NAME, fk->name().c_str());

  std::string col_list;
  std::string ref_table;
  std::string ref_col_list;
  std::string on_update;
  std::string on_delete;

  get_fk_desc(fk, col_list, ref_table, ref_col_list, on_update, on_delete);

  f->SetValue(kbtr_TABLE_FK_COLUMNS, col_list);
  f->SetValue(kbtr_TABLE_FK_REF_TABLE, ref_table);
  f->SetValue(kbtr_TABLE_FK_REF_COLUMNS, ref_col_list);
  f->SetValue(kbtr_TABLE_FK_ON_UPDATE, on_update);
  f->SetValue(kbtr_TABLE_FK_ON_DELETE, on_delete);
}

void ActionGenerateReport::alter_table_drop_fk(db_mysql_ForeignKeyRef fk)
{
  TemplateDictionary *f= curr_table->AddSectionDictionary(kbtr_TABLE_FK_REMOVED);
  f->SetValue(kbtr_TABLE_FK_NAME, fk->name().c_str());
}

void ActionGenerateReport::alter_table_fks_end(db_mysql_TableRef table)
{
  if(table->foreignKeys().count() > 0)
    curr_table->AddSectionDictionary(kbtr_ALTER_TABLE_FKS_FOOTER);
}


// triggers create/drop
void ActionGenerateReport::create_trigger(db_mysql_TriggerRef trigger, bool for_alter)
{
  TemplateDictionary *t= dict.AddSectionDictionary(kbtr_CREATE_TRIGGER);

  t->SetValue(kbtr_CREATE_TRIGGER_NAME, trigger_name(trigger));
}

void ActionGenerateReport::drop_trigger(db_mysql_TriggerRef trigger, bool for_alter)
{
  TemplateDictionary *t= dict.AddSectionDictionary(kbtr_DROP_TRIGGER);

  t->SetValue(kbtr_DROP_TRIGGER_NAME, trigger_name(trigger));
}

// views create/drop
void ActionGenerateReport::create_view(db_mysql_ViewRef view)
{
  TemplateDictionary *v= dict.AddSectionDictionary(kbtr_CREATE_VIEW);
  v->SetValue(kbtr_CREATE_VIEW_NAME, object_name(view));
}

void ActionGenerateReport::drop_view(db_mysql_ViewRef view)
{
  TemplateDictionary *v= dict.AddSectionDictionary(kbtr_DROP_VIEW);
  v->SetValue(kbtr_DROP_VIEW_NAME, object_name(view));
}

// routines create/drop
void ActionGenerateReport::create_routine(db_mysql_RoutineRef routine, bool for_alter)
{
  TemplateDictionary *r= dict.AddSectionDictionary(kbtr_CREATE_ROUTINE);
  r->SetValue(kbtr_CREATE_ROUTINE_NAME, object_name(routine));
}

void ActionGenerateReport::drop_routine(db_mysql_RoutineRef routine, bool for_alter)
{
  TemplateDictionary *r= dict.AddSectionDictionary(kbtr_DROP_ROUTINE);
  r->SetValue(kbtr_DROP_ROUTINE_NAME, object_name(routine));
}


// users create/drop
void ActionGenerateReport::create_user(db_UserRef user)
{
  TemplateDictionary *u= dict.AddSectionDictionary(kbtr_CREATE_USER);
  u->SetValue(kbtr_CREATE_USER_NAME, object_name(user));
}

void ActionGenerateReport::drop_user(db_UserRef user)
{
  TemplateDictionary *u= dict.AddSectionDictionary(kbtr_DROP_USER);
  u->SetValue(kbtr_DROP_USER_NAME, object_name(user));
}

// schema create/drop
void ActionGenerateReport::create_schema(db_mysql_SchemaRef schema)
{
  TemplateDictionary *s= dict.AddSectionDictionary(kbtr_CREATE_SCHEMA);
  s->SetValue(kbtr_CREATE_SCHEMA_NAME, object_name(schema));
}

void ActionGenerateReport::drop_schema(db_mysql_SchemaRef schema)
{
  TemplateDictionary *s= dict.AddSectionDictionary(kbtr_DROP_SCHEMA);
  s->SetValue(kbtr_DROP_SCHEMA_NAME, object_name(schema));
}

// alter schema
void ActionGenerateReport::alter_schema_props_begin(db_mysql_SchemaRef)
{
  curr_schema= NULL;
}

void ActionGenerateReport::alter_schema_name(db_mysql_SchemaRef schema, grt::StringRef value)
{
  if(curr_schema == NULL)
  {
    curr_schema= dict.AddSectionDictionary(kbtr_ALTER_SCHEMA);
    curr_schema->SetValue(kbtr_ALTER_SCHEMA_NAME, object_name(schema));
  }

  TemplateDictionary *c= curr_schema->AddSectionDictionary(kbtr_ALTER_SCHEMA_NAME);
  c->SetValue(kbtr_OLD_SCHEMA_NAME, schema->name().c_str());
  c->SetValue(kbtr_NEW_SCHEMA_NAME, value.c_str());
}

void ActionGenerateReport::alter_schema_default_charset(db_mysql_SchemaRef schema, grt::StringRef value)
{
  if(curr_schema == NULL)
  {
    curr_schema= dict.AddSectionDictionary(kbtr_ALTER_SCHEMA);
    curr_schema->SetValue(kbtr_ALTER_SCHEMA_NAME, object_name(schema));
  }

  TemplateDictionary *c= curr_schema->AddSectionDictionary(kbtr_ALTER_SCHEMA_CHARSET);
  c->SetValue(kbtr_OLD_SCHEMA_CHARSET, schema->defaultCharacterSetName().c_str());
  c->SetValue(kbtr_NEW_SCHEMA_CHARSET, value.c_str());
  //schema_altered= true;
}

void ActionGenerateReport::alter_schema_default_collate(db_mysql_SchemaRef schema, grt::StringRef value)
{
  if(curr_schema == NULL)
  {
    curr_schema= dict.AddSectionDictionary(kbtr_ALTER_SCHEMA);
    curr_schema->SetValue(kbtr_ALTER_SCHEMA_NAME, object_name(schema));
  }

  TemplateDictionary *c= curr_schema->AddSectionDictionary(kbtr_ALTER_SCHEMA_COLLATE);
  c->SetValue(kbtr_OLD_SCHEMA_COLLATE, schema->defaultCollationName().c_str());
  c->SetValue(kbtr_NEW_SCHEMA_COLLATE, value.c_str());
  //schema_altered= true;
}

void ActionGenerateReport::alter_schema_props_end(db_mysql_SchemaRef schema)
{
}

