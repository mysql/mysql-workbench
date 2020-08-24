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

#include "db_mysql_diffsqlgen.h"

#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"

#include "db_mysql_diffsqlgen_grant.h"

#include "grt/common.h"

#include <algorithm>
#include <ctype.h>

#include "module_db_mysql.h"
#include "module_db_mysql_shared_code.h"

void DiffSQLGeneratorBE::generate_set_partitioning(db_mysql_TableRef table, const grt::DiffChange *table_diffchange) {
  bool part_type_set = false, part_expr_set = false, subpart_type_set = false, subpart_expr_set = false,
       part_count_set = false, part_defs_set = false;
  std::string part_type, part_expr, subpart_type, subpart_expr;
  grt::ListRef<db_mysql_PartitionDefinition> part_defs(true);
  ssize_t part_count = 0;

  // gather all relevant attributes from change object
  const grt::ChangeSet *cs = table_diffchange->subchanges();
  for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
    const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());

    if (attr_change->get_attr_name().compare("partitionType") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      part_type.assign(grt::StringRef::cast_from(change->get_new_value()));
      part_type_set = true;
    } else if (attr_change->get_attr_name().compare("partitionExpression") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      part_expr.assign(grt::StringRef::cast_from(change->get_new_value()));
      part_expr_set = true;
    } else if (attr_change->get_attr_name().compare("subpartitionType") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      subpart_type.assign(grt::StringRef::cast_from(change->get_new_value()));
      subpart_type_set = true;
    } else if (attr_change->get_attr_name().compare("subpartitionExpression") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      subpart_expr.assign(grt::StringRef::cast_from(change->get_new_value()));
      subpart_expr_set = true;
    } else if (attr_change->get_attr_name().compare("partitionCount") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      part_count = grt::IntegerRef::cast_from(change->get_new_value());
      part_count_set = true;
    } else if (attr_change->get_attr_name().compare("partitionDefinitions") == 0) {
      const grt::MultiChange *change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      const grt::ChangeSet *part_defs_cs = change->subchanges();

      for (grt::ChangeSet::const_iterator part_defs_e = part_defs_cs->end(), part_defs_it = part_defs_cs->begin();
           part_defs_it != part_defs_e; part_defs_it++) {
        grt::ValueRef part_def_grt_value;

        const std::shared_ptr<grt::DiffChange> part_def_change = *part_defs_it;
        if (part_def_change->get_change_type() == grt::ListItemAdded) {
          const grt::ListItemAddedChange *added_change =
            static_cast<const grt::ListItemAddedChange *>(part_def_change.get());

          part_def_grt_value = added_change->get_value();
        } else if (part_def_change->get_change_type() == grt::ListItemModified) {
          const grt::ListItemModifiedChange *modified_change =
            static_cast<const grt::ListItemModifiedChange *>(part_def_change.get());

          part_def_grt_value = modified_change->get_new_value();
        }

        db_mysql_PartitionDefinitionRef part_def_value(db_mysql_PartitionDefinitionRef::cast_from(part_def_grt_value));

        if (part_def_value.is_valid())
          part_defs.insert(part_def_value);
      }
      part_defs_set = true;
    }
  }

  if (!part_type_set)
    part_type.assign(table->partitionType().c_str());

  if (!part_expr_set)
    part_expr.assign(table->partitionExpression().c_str());

  if (!subpart_type_set)
    subpart_type.assign(table->subpartitionType().c_str());

  if (!subpart_expr_set)
    subpart_expr.assign(table->subpartitionExpression().c_str());

  if (!part_count_set)
    part_count = table->partitionCount();

  if (!part_defs_set)
    part_defs = table->partitionDefinitions();

  callback->alter_table_generate_partitioning(table, part_type, part_expr, (int)part_count, subpart_type, subpart_expr,
                                              part_defs);
}

void DiffSQLGeneratorBE::generate_create_partitioning(db_mysql_TableRef table) {
  callback->alter_table_generate_partitioning(
    table, std::string(table->partitionType().is_valid() ? table->partitionType().c_str() : ""),
    std::string(table->partitionExpression().is_valid() ? table->partitionExpression().c_str() : ""),
    (int)table->partitionCount(),
    std::string(table->subpartitionType().is_valid() ? table->subpartitionType().c_str() : ""),
    std::string(table->subpartitionExpression().is_valid() ? table->subpartitionExpression().c_str() : ""),
    table->partitionDefinitions());
}

void DiffSQLGeneratorBE::generate_create_stmt(db_mysql_TableRef table) {
  if (table->isStub())
    return;

  bool process_table = true;
  std::string table_name_for_filter(get_old_object_name_for_key(table, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_tables.find(table_name_for_filter) == _filtered_tables.end())
      process_table = false;

  if (process_table) {
    callback->create_table_props_begin(table);

    // note: for TIMESTAMP columns 'ON UPDATE' are stored in defaultValue field
    // see http://dev.mysql.com/doc/refman/5.1/en/timestamp.html for more info

    callback->create_table_columns_begin(table);

    grt::ListRef<db_mysql_Column> columns = table->columns();
    for (size_t columns_count = columns.count(), i = 0; i < columns_count; i++) {
      db_mysql_ColumnRef column = columns.get(i);
      callback->create_table_column(column);
    }

    callback->create_table_columns_end(table);

    // indices processing

    callback->create_table_indexes_begin(table);

    std::set<db_IndexRef> fk_indexes;
    std::set<db_IndexRef> fk_create_indexes;
    std::set<db_IndexRef> fk_skip_indexes;
    for (size_t fk_count = table->foreignKeys().count(), i = 0; i < fk_count; i++) {
      fk_indexes.insert(table->foreignKeys()[i]->index());
      (table->foreignKeys()[i]->modelOnly() ? fk_skip_indexes : fk_create_indexes)
        .insert(table->foreignKeys()[i]->index());
    }

    grt::ListRef<db_mysql_Index> indices = table->indices();
    for (size_t index_count = indices.count(), i = 0; i < index_count; i++) {
      db_mysql_IndexRef index = indices.get(i);

      if (index->isPrimary()) {
        callback->create_table_index(index, false);
        continue;
      }

      if (_skip_fk_indexes &&
          (strcasecmp(index->indexType().c_str(), "FOREIGN") == 0 || fk_indexes.find(index) != fk_indexes.end()))
        continue;

      if ((fk_skip_indexes.find(index) != fk_skip_indexes.end()) &&
          (fk_create_indexes.find(index) == fk_create_indexes.end()))
        continue;
      callback->create_table_index(index, _gen_create_index);
    }

    callback->create_table_indexes_end(table);

    callback->create_table_fks_begin(table);

    if (!_skip_foreign_keys) {
      grt::ListRef<db_mysql_ForeignKey> fks = table->foreignKeys();
      for (size_t fk_count = fks.count(), i = 0; i < fk_count; i++) {
        db_mysql_ForeignKeyRef fk = fks.get(i);
        if (fk.is_valid() && fk->referencedTable().is_valid()) {
          if (fk->modelOnly() || fk->referencedTable()->modelOnly())
            continue;

          callback->create_table_fk(fk);
        }
      }
    }
    callback->create_table_fks_end(table);

    // TODO: [TABLESPACE tablespace_name STORAGE DISK]

    if (strlen(table->tableEngine().c_str()))
      callback->create_table_engine(table->tableEngine());

    if (strlen(table->nextAutoInc().c_str()))
      callback->create_table_next_auto_inc(table->nextAutoInc());

    if (strlen(table->avgRowLength().c_str()))
      callback->create_table_avg_row_length(table->avgRowLength());

    if (table->checksum() != 0)
      callback->create_table_checksum(table->checksum());

    // Set a collation only if it differs from the default collation for that charset.
    // Ignore the collation if it doesn't match the charset, however.
    if (table->defaultCharacterSetName().is_valid()) {
      std::string charset = table->defaultCharacterSetName();
      if (!charset.empty()) {
        callback->create_table_charset(charset);

        if (table->defaultCollationName().is_valid()) {
          std::string collation = table->defaultCollationName();
          if (!collation.empty() && (charsetForCollation(collation) == charset) &&
              (defaultCollationForCharset(table->defaultCharacterSetName()) != collation))
            callback->create_table_collate(collation);
        }
      }
    }

    if (strlen(table->comment().c_str()))
      callback->create_table_comment(table->comment());

    // CONNECTION [=] 'connect_string'

    if (strlen(table->tableDataDir().c_str()))
      callback->create_table_data_dir(table->tableDataDir());

    if (table->delayKeyWrite() != 0)
      callback->create_table_delay_key_write(table->delayKeyWrite());

    if (strlen(table->tableIndexDir().c_str()))
      callback->create_table_index_dir(table->tableIndexDir());

    if (strlen(table->mergeInsert().c_str()))
      callback->create_table_merge_insert(table->mergeInsert());

    // KEY_BLOCK_SIZE [=] value

    if (strlen(table->maxRows().c_str()))
      callback->create_table_max_rows(table->maxRows());

    if (strlen(table->minRows().c_str()))
      callback->create_table_min_rows(table->minRows());

    if (strlen(table->packKeys().c_str()))
      callback->create_table_pack_keys(table->packKeys());

    if (strlen(table->password().c_str()))
      callback->create_table_password(table->password());

    if (strlen(table->rowFormat().c_str()))
      callback->create_table_row_format(table->rowFormat());

    if (strlen(table->keyBlockSize().c_str()))
      callback->create_table_key_block_size(table->keyBlockSize());

    if (strlen(table->mergeUnion().c_str()))
      callback->create_table_merge_union(table->mergeUnion());

    if (table->partitionType().is_valid() && strlen(table->partitionType().c_str()) &&
        table->partitionExpression().is_valid() /*&& strlen(table->partitionExpression().c_str())*/) {
      generate_create_partitioning(table);
    }

    callback->create_table_props_end(table);
  } // process_table

  grt::ListRef<db_mysql_Trigger> triggers = table->triggers();
  for (size_t c = triggers.count(), i = 0; i < c; i++) {
    db_mysql_TriggerRef trigger = triggers.get(i);
    generate_create_stmt(trigger);
  }
}

void DiffSQLGeneratorBE::generate_create_stmt(db_mysql_ViewRef view) {
  std::string view_name_for_filter(get_old_object_name_for_key(view, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_views.find(view_name_for_filter) == _filtered_views.end())
      return;

  callback->create_view(view);
}

void DiffSQLGeneratorBE::generate_create_stmt(db_mysql_RoutineRef routine, bool for_alter) {
  std::string routine_name_for_filter(get_old_object_name_for_key(routine, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_routines.find(routine_name_for_filter) == _filtered_routines.end())
      return;

  callback->create_routine(routine, for_alter);
}

void DiffSQLGeneratorBE::generate_create_stmt(db_mysql_TriggerRef trigger, bool for_alter) {
  std::string trigger_name_for_filter(get_old_object_name_for_key(trigger, _case_sensitive));

  if (_use_filtered_lists)
    if (_filtered_triggers.find(trigger_name_for_filter) == _filtered_triggers.end())
      return;

  callback->create_trigger(trigger, for_alter);
}

void DiffSQLGeneratorBE::generate_create_stmt(db_mysql_SchemaRef schema) {
  std::string schema_name_for_filter(get_old_object_name_for_key(schema, _case_sensitive));

  if (_use_filtered_lists)
    if (_filtered_schemata.find(schema_name_for_filter) == _filtered_schemata.end())
      return;

  callback->create_schema(schema);

  grt::ListRef<db_mysql_Table> tables = schema->tables();
  for (size_t count = tables.count(), i = 0; i < count; i++) {
    db_mysql_TableRef table = tables.get(i);
    generate_create_stmt(table);
  }

  grt::ListRef<db_mysql_View> views = schema->views();
  for (size_t count = views.count(), i = 0; i < count; i++) {
    db_mysql_ViewRef view = views.get(i);
    generate_create_stmt(view);
  }

  grt::ListRef<db_mysql_Routine> routines = schema->routines();
  for (size_t count = routines.count(), i = 0; i < count; i++) {
    db_mysql_RoutineRef routine = routines.get(i);
    generate_create_stmt(routine);
  }
}

void DiffSQLGeneratorBE::generate_create_stmt(db_UserRef user) {
  std::string user_name_for_filter(get_old_object_name_for_key(user, _case_sensitive));

  if (_use_filtered_lists)
    if (_filtered_users.find(user_name_for_filter) == _filtered_users.end())
      return;

  callback->create_user(user);
}

void DiffSQLGeneratorBE::generate_create_stmt(db_mysql_CatalogRef catalog) {
  grt::ListRef<db_mysql_Schema> schemata = catalog->schemata();
  for (size_t count = schemata.count(), i = 0; i < count; i++) {
    db_mysql_SchemaRef schema = schemata.get(i);
    generate_create_stmt(schema);
  }

  for (size_t count = catalog->users().count(), i = 0; i < count; i++) {
    db_UserRef user = catalog->users().get(i);
    generate_create_stmt(user);
  }
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_mysql_TableRef table) {
  if (table->isStub())
    return;

  std::string table_name_for_filter(get_old_object_name_for_key(table, _case_sensitive));
  if (!(_use_filtered_lists && (_filtered_tables.find(table_name_for_filter) == _filtered_tables.end())))
    callback->drop_table(table);

  grt::ListRef<db_mysql_Trigger> triggers = table->triggers();
  for (size_t c = triggers.count(), i = 0; i < c; i++) {
    db_mysql_TriggerRef trigger = triggers.get(i);
    generate_drop_stmt(trigger);
  }
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_mysql_TriggerRef trigger, bool for_alter) {
  std::string trigger_name_for_filter(get_old_object_name_for_key(trigger, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_triggers.find(trigger_name_for_filter) == _filtered_triggers.end())
      return;

  callback->drop_trigger(trigger, for_alter);
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_mysql_ViewRef view) {
  std::string view_name_for_filter(get_old_object_name_for_key(view, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_views.find(view_name_for_filter) == _filtered_views.end())
      return;

  callback->drop_view(view);
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_mysql_RoutineRef routine, bool for_alter) {
  std::string routine_name_for_filter(get_old_object_name_for_key(routine, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_routines.find(routine_name_for_filter) == _filtered_routines.end())
      return;

  callback->drop_routine(routine, for_alter);
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_UserRef user) {
  callback->drop_user(user);
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_mysql_SchemaRef schema) {
  callback->drop_schema(schema);
  callback->disable_list_insert(true);
  grt::ListRef<db_mysql_Table> tables = schema->tables();
  for (size_t count = tables.count(), i = 0; i < count; i++) {
    db_mysql_TableRef table = tables.get(i);
    generate_drop_stmt(table);
  }

  grt::ListRef<db_mysql_View> views = schema->views();
  for (size_t count = views.count(), i = 0; i < count; i++) {
    db_mysql_ViewRef view = views.get(i);
    generate_drop_stmt(view);
  }

  grt::ListRef<db_mysql_Routine> routines = schema->routines();
  for (size_t count = routines.count(), i = 0; i < count; i++) {
    db_mysql_RoutineRef routine = routines.get(i);
    generate_drop_stmt(routine);
  }
  callback->disable_list_insert(false);
}

void DiffSQLGeneratorBE::generate_drop_stmt(db_mysql_CatalogRef catalog) {
  grt::ListRef<db_mysql_Schema> schemata = catalog->schemata();
  for (size_t count = schemata.count(), i = 0; i < count; i++) {
    db_mysql_SchemaRef schema = schemata.get(i);
    generate_drop_stmt(schema);
  }

  for (size_t count = catalog->users().count(), i = 0; i < count; i++) {
    db_UserRef user = catalog->users().get(i);
    generate_drop_stmt(user);
  }
}

void DiffSQLGeneratorBE::generate_alter(grt::ListRef<db_mysql_Column> columns, const grt::MultiChange *diffchange) {
  const grt::ChangeSet *columns_cs = diffchange->subchanges();

  if (columns.count() == 0)
    return;

  db_mysql_TableRef table = db_mysql_TableRef::cast_from(columns.get(0)->owner());

  callback->alter_table_columns_begin(table);

  Column_rename_map column_rename_map;

  // build a map of column renames
  // process CHANGE COLUMN (handles content change only)
  for (grt::ChangeSet::const_iterator e = columns_cs->end(), it = columns_cs->begin(); it != e; it++) {
    const grt::DiffChange* column_change = &(*it->get());

    if (column_change->get_change_type() == grt::ListItemModified) {
      const grt::ListItemModifiedChange *modified_change =
        static_cast<const grt::ListItemModifiedChange *>(column_change);
      db_mysql_ColumnRef column = db_mysql_ColumnRef::cast_from(grt::ValueRef(modified_change->get_old_value()));
      if (strcmp(column->name().c_str(), column->oldName().c_str()) != 0)
        column_rename_map[std::string(column->oldName().c_str())] = std::string(column->name().c_str());
    } else if (column_change->get_change_type() == grt::ListItemOrderChanged) {
      const grt::ListItemOrderChange *order_change = static_cast<const grt::ListItemOrderChange *>(column_change);
      db_mysql_ColumnRef org_col = db_mysql_ColumnRef::cast_from(grt::ValueRef(order_change->get_old_value()));
      db_mysql_ColumnRef mod_col = db_mysql_ColumnRef::cast_from(grt::ValueRef(order_change->get_new_value()));
      if (strcmp(org_col->name().c_str(), mod_col->oldName().c_str()) != 0)
        column_rename_map[std::string(org_col->oldName().c_str())] = std::string(org_col->name().c_str());
    }
  }

  // process DROP COLUMN
  for (grt::ChangeSet::const_iterator e = columns_cs->end(), it = columns_cs->begin(); it != e; it++) {
    const grt::DiffChange* column_change = &(*it->get());
    if (column_change->get_change_type() != grt::ListItemRemoved)
      continue;

    const grt::ListItemRemovedChange *removed_change =
      static_cast<const grt::ListItemRemovedChange *>(column_change);
    callback->alter_table_drop_column(table, db_mysql_ColumnRef::cast_from(removed_change->get_value()));
  }
    
  // process ADD COLUMN
  for (grt::ChangeSet::const_iterator e = columns_cs->end(), it = columns_cs->begin(); it != e; it++) {
    const grt::DiffChange* column_change = &(*it->get());
    if (column_change->get_change_type() != grt::ListItemAdded)
      continue;
    
    const grt::ListItemAddedChange *added_change = static_cast<const grt::ListItemAddedChange *>(column_change);
    
    callback->alter_table_add_column(table, column_rename_map,
                                     db_mysql_ColumnRef::cast_from(grt::ValueRef(added_change->get_value())),
                                     db_mysql_ColumnRef::cast_from(added_change->get_prev_item()));
  }

  // process CHANGE COLUMN (handles both position and content change)
  for (grt::ChangeSet::const_iterator e = columns_cs->end(), it = columns_cs->begin(); it != e; it++) {
    const grt::DiffChange* column_change = &(*it->get());

    if (column_change->get_change_type() != grt::ListItemOrderChanged)
      continue;

    const grt::ListItemOrderChange *order_change = static_cast<const grt::ListItemOrderChange *>(column_change);
    // index_pair.second is the new position
    callback->alter_table_change_column(
      table, db_mysql_ColumnRef::cast_from(grt::ValueRef(order_change->get_old_value())),
      db_mysql_ColumnRef::cast_from(grt::ValueRef(order_change->get_new_value())),
      //      v == NULL ? db_mysql_ColumnRef() : db_mysql_ColumnRef::cast_from(grt::ValueRef(*v)),
      db_mysql_ColumnRef::cast_from(order_change->get_prev_item()), false, column_rename_map);
  }

  // process CHANGE COLUMN (handles content change only)
  for (grt::ChangeSet::const_iterator e = columns_cs->end(), it = columns_cs->begin(); it != e; it++) {
    const grt::DiffChange* column_change = &(*it->get());

    if (column_change->get_change_type() != grt::ListItemModified)
      continue;

    const grt::ListItemModifiedChange *modified_change =
      static_cast<const grt::ListItemModifiedChange *>(column_change);

    callback->alter_table_change_column(table,
                                        db_mysql_ColumnRef::cast_from(grt::ValueRef(modified_change->get_new_value())),
                                        db_mysql_ColumnRef(), db_mysql_ColumnRef(), true, column_rename_map);
  }

  callback->alter_table_columns_end(table);
}

void DiffSQLGeneratorBE::generate_alter(grt::ListRef<db_mysql_Index> indices, const grt::MultiChange *diffchange) {
  const grt::ChangeSet *indices_cs = diffchange->subchanges();

  for (grt::ChangeSet::const_iterator e = indices_cs->end(), it = indices_cs->begin(); it != e; it++) {
    const std::shared_ptr<grt::DiffChange> index_change = *it;

    switch (index_change->get_change_type()) {
      case grt::ListItemAdded: // ADD INDEX
      {
        const grt::ListItemAddedChange *added_change =
          static_cast<const grt::ListItemAddedChange *>(index_change.get());
        callback->alter_table_add_index(db_mysql_IndexRef::cast_from(added_change->get_value()));
      } break;
      case grt::ListItemRemoved: // DROP INDEX
      {
        const grt::ListItemRemovedChange *removed_change =
          static_cast<const grt::ListItemRemovedChange *>(index_change.get());
        callback->alter_table_drop_index(db_mysql_IndexRef::cast_from(removed_change->get_value()));
      } break;
      case grt::ListItemOrderChanged: // DROP/ADD INDEX
      {
        const grt::ListItemOrderChange *order_change =
          static_cast<const grt::ListItemOrderChange *>(index_change.get());
        // Only position changed nothing to do
        if (!order_change->get_subchange())
          break;
        callback->alter_table_drop_index(db_mysql_IndexRef::cast_from(order_change->get_old_value()));
        callback->alter_table_add_index(db_mysql_IndexRef::cast_from(order_change->get_new_value()));
      } break;

      case grt::ListItemModified: {
        const grt::ListItemModifiedChange *modified_change =
          static_cast<const grt::ListItemModifiedChange *>(index_change.get());

        callback->alter_table_change_index(db_mysql_IndexRef::cast_from(modified_change->get_old_value()),
                                           db_mysql_IndexRef::cast_from(modified_change->get_new_value()));
      } break;
      default:
        break;
    }
  }
}

void DiffSQLGeneratorBE::generate_alter_drop(grt::ListRef<db_mysql_ForeignKey> fks,
                                             const grt::MultiChange *diffchange) {
  /*
    You cannot add a foreign key and drop a foreign key in separate
    clauses of a single ALTER TABLE  statement. You must use separate
    statements.
  */

  const grt::ChangeSet *fks_cs = diffchange->subchanges();

  for (grt::ChangeSet::const_iterator e = fks_cs->end(), it = fks_cs->begin(); it != e; it++) {
    const std::shared_ptr<grt::DiffChange> fk_change = *it;

    db_mysql_ForeignKeyRef fk1;
    switch (fk_change->get_change_type()) {
      case grt::ListItemRemoved: // DROP FK
      {
        const grt::ListItemRemovedChange *removed_change =
          static_cast<const grt::ListItemRemovedChange *>(fk_change.get());
        fk1 = db_mysql_ForeignKeyRef::cast_from(removed_change->get_value());
      } break;
      case grt::ListItemModified: {
        const grt::ListItemModifiedChange *modified_change =
          static_cast<const grt::ListItemModifiedChange *>(fk_change.get());
        fk1 = db_mysql_ForeignKeyRef::cast_from(modified_change->get_old_value());
      } break;
      default:
        break;
    }
    if (fk1.is_valid() &&
        (fk1->modelOnly() || !fk1->referencedTable().is_valid() || fk1->referencedTable()->modelOnly()))
      continue;

    switch (fk_change->get_change_type()) {
      case grt::ListItemRemoved: // DROP FK
      {
        const grt::ListItemRemovedChange *removed_change =
          static_cast<const grt::ListItemRemovedChange *>(fk_change.get());
        callback->alter_table_drop_fk(db_mysql_ForeignKeyRef::cast_from(removed_change->get_value()));
      } break;
      case grt::ListItemModified: {
        const grt::ListItemModifiedChange *modified_change =
          static_cast<const grt::ListItemModifiedChange *>(fk_change.get());
        callback->alter_table_drop_fk(db_mysql_ForeignKeyRef::cast_from(modified_change->get_old_value()));
      } break;
      default:
        break;
    }
  }
}

void DiffSQLGeneratorBE::generate_alter(grt::ListRef<db_mysql_ForeignKey> fks, const grt::MultiChange *diffchange) {
  /*
    You cannot add a foreign key and drop a foreign key in separate
    clauses of a single ALTER TABLE  statement. You must use separate
    statements.
  */

  const grt::ChangeSet *fks_cs = diffchange->subchanges();

  for (grt::ChangeSet::const_iterator e = fks_cs->end(), it = fks_cs->begin(); it != e; it++) {
    const std::shared_ptr<grt::DiffChange> fk_change = *it;

    db_mysql_ForeignKeyRef fk1, fk2;
    switch (fk_change->get_change_type()) {
      case grt::ListItemAdded: // ADD FK
      {
        const grt::ListItemAddedChange *added_change = static_cast<const grt::ListItemAddedChange *>(fk_change.get());
        fk1 = db_mysql_ForeignKeyRef::cast_from(added_change->get_value());
      } break;
      case grt::ListItemRemoved: // DROP FK
      {
        const grt::ListItemRemovedChange *removed_change =
          static_cast<const grt::ListItemRemovedChange *>(fk_change.get());
        fk1 = db_mysql_ForeignKeyRef::cast_from(removed_change->get_value());
      } break;
      case grt::ListItemOrderChanged: // DROP/ADD FK
      {
        const grt::ListItemOrderChange *order_change = static_cast<const grt::ListItemOrderChange *>(fk_change.get());
        // Only position changed nothing to do
        if (!order_change->get_subchange())
          break;
        fk1 = db_mysql_ForeignKeyRef::cast_from(order_change->get_old_value());
        fk2 = db_mysql_ForeignKeyRef::cast_from(order_change->get_new_value());
      } break;
      case grt::ListItemModified: {
        const grt::ListItemModifiedChange *modified_change =
          static_cast<const grt::ListItemModifiedChange *>(fk_change.get());
        fk1 = db_mysql_ForeignKeyRef::cast_from(modified_change->get_old_value());
        fk2 = db_mysql_ForeignKeyRef::cast_from(modified_change->get_new_value());
      } break;
      default:
        break;
    }
    if (fk1.is_valid() &&
        (fk1->modelOnly() || !fk1->referencedTable().is_valid() || fk1->referencedTable()->modelOnly()))
      continue;
    if (fk2.is_valid() &&
        (fk2->modelOnly() || !fk2->referencedTable().is_valid() || fk2->referencedTable()->modelOnly()))
      continue;

    switch (fk_change->get_change_type()) {
      case grt::ListItemAdded: // ADD FK
      {
        const grt::ListItemAddedChange *added_change = static_cast<const grt::ListItemAddedChange *>(fk_change.get());
        callback->alter_table_add_fk(db_mysql_ForeignKeyRef::cast_from(added_change->get_value()));
      } break;
      case grt::ListItemRemoved: // DROP FK
                                 /*      {
                                         const grt::ListItemRemovedChange *removed_change= static_cast<const grt::ListItemRemovedChange
                                    *>(fk_change);
                                         callback->alter_table_drop_fk(db_mysql_ForeignKeyRef::cast_from(removed_change->get_old_value()));
                                       }*/
        break;
      case grt::ListItemOrderChanged: // DROP/ADD FK
      {
        const grt::ListItemOrderChange *order_change = static_cast<const grt::ListItemOrderChange *>(fk_change.get());
        // Only position changed nothing to do
        if (!order_change->get_subchange())
          break;
        callback->alter_table_drop_fk(db_mysql_ForeignKeyRef::cast_from(order_change->get_old_value()));
        callback->alter_table_add_fk(db_mysql_ForeignKeyRef::cast_from(order_change->get_new_value()));
      } break;

      case grt::ListItemModified: {
        const grt::ListItemModifiedChange *modified_change =
          static_cast<const grt::ListItemModifiedChange *>(fk_change.get());
        //        callback->alter_table_drop_fk(db_mysql_ForeignKeyRef::cast_from(modified_change->get_old_value()));
        callback->alter_table_add_fk(db_mysql_ForeignKeyRef::cast_from(modified_change->get_new_value()));
      } break;
      default:
        break;
    }
  }
}

void DiffSQLGeneratorBE::generate_alter_stmt_drops(db_mysql_TableRef table, const grt::DiffChange *diffchange) {
  if (table->isStub())
    return;

  bool process_table = true;

  std::string table_name_for_filter(get_old_object_name_for_key(table, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_tables.find(table_name_for_filter) == _filtered_tables.end())
      process_table = false;
  if (!process_table)
    return;
  const grt::ChangeSet *cs = diffchange->subchanges();
  bool gen_alter = false;

  for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
    const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());

    if (attr_change->get_attr_name().compare("foreignKeys") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      if (!gen_alter) {
        gen_alter = true;
        callback->alter_table_props_begin(table);
      }

      callback->alter_table_fks_begin(table);
      generate_alter_drop(table->foreignKeys(), list_change);
      callback->alter_table_fks_end(table);
    }
  }
  if (gen_alter)
    callback->alter_table_props_end(table);
}

void DiffSQLGeneratorBE::generate_alter_stmt(db_mysql_TableRef table, const grt::DiffChange *diffchange,
                                             AlterTableFlags alter_table_flags) {
  if (table->isStub())
    return;

  const grt::ChangeSet *cs = diffchange->subchanges();

  if ((alter_table_flags & EverythingButForeignKeys)) {
    // process table triggers
    for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
      const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());

      if (attr_change->get_attr_name().compare("triggers") == 0) {
        const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());

        const grt::ChangeSet *triggers_cs = list_change->subchanges();

        for (grt::ChangeSet::const_iterator e2 = triggers_cs->end(), jt = triggers_cs->begin(); jt != e2; jt++) {
          const grt::DiffChange *trigger_change = jt->get();

          switch (trigger_change->get_change_type()) {
            case grt::ListItemAdded: {
              db_mysql_TriggerRef trigger(db_mysql_TriggerRef::cast_from(
                static_cast<const grt::ListItemAddedChange *>(trigger_change)->get_value()));
              generate_create_stmt(trigger, true);
            } break;
            case grt::ListItemRemoved: {
              db_mysql_TriggerRef trigger(db_mysql_TriggerRef::cast_from(
                static_cast<const grt::ListItemRemovedChange *>(trigger_change)->get_value()));
              generate_drop_stmt(trigger, true);
            } break;
            case grt::ListItemModified: {
              db_mysql_TriggerRef old_trigger = db_mysql_TriggerRef::cast_from(
                grt::ValueRef(static_cast<const grt::ListItemModifiedChange *>(trigger_change)->get_old_value()));
              db_mysql_TriggerRef new_trigger = db_mysql_TriggerRef::cast_from(
                grt::ValueRef(static_cast<const grt::ListItemModifiedChange *>(trigger_change)->get_new_value()));

              generate_drop_stmt(old_trigger, true);
              generate_create_stmt(new_trigger, true);
            } break;
            case grt::ListItemOrderChanged: {
              const grt::ListItemOrderChange *order_change =
                dynamic_cast<const grt::ListItemOrderChange *>(trigger_change);

              if (order_change) // && order_change->get_subchange())
              {
                //                const grt::ListItemModifiedChange *change = dynamic_cast<const
                //                grt::ListItemModifiedChange *>(order_change->get_subchange().get());

                //                if (change)
                {
                  generate_drop_stmt(db_mysql_TriggerRef::cast_from(order_change->get_old_value()), true);
                  generate_create_stmt(db_mysql_TriggerRef::cast_from(order_change->get_new_value()), true);
                }
              }
            }
            default:
              break;
          }
        }
      }
    }
  }

  const std::string table_name_for_filter(get_old_object_name_for_key(table, _case_sensitive));
  if (_use_filtered_lists && (_filtered_tables.find(table_name_for_filter) == _filtered_tables.end()))
    return;

  bool partitions_processed = false;
  cs = diffchange->subchanges();

  // check whether all changes detected for this table are dummy changes (index reorders or fk reorders)
  bool dummy_changes_only = true;
  for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
    const grt::ObjectAttrModifiedChange *attr_change = dynamic_cast<const grt::ObjectAttrModifiedChange *>(it->get());

    if (attr_change->get_attr_name().compare("indices") == 0 ||
        attr_change->get_attr_name().compare("foreignKeys") == 0) {
      const grt::MultiChange *list_change = dynamic_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      // ignore changes if they're only reorderings (with no subchanges)
      if (list_change && list_change->subchanges()) {
        const grt::ListItemOrderChange *order_change = NULL;
        if (list_change->subchanges()->changes.size() == 1)
          order_change = dynamic_cast<const grt::ListItemOrderChange *>(list_change->subchanges()->begin()->get());
        if (!order_change || order_change->get_subchange()) {
          dummy_changes_only = false;
          break;
        }
      }
    } else
      dummy_changes_only = false;
  }
  if (dummy_changes_only)
    return;

  callback->alter_table_props_begin(table);

  for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
    const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());

    if (!(alter_table_flags & OnlyForeignKeys) && attr_change->get_attr_name().compare("foreignKeys") == 0)
      continue;
    else if (!(alter_table_flags & EverythingButForeignKeys) &&
             attr_change->get_attr_name().compare("foreignKeys") != 0)
      continue;

    if (attr_change->get_attr_name().compare("name") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_name(table, str);
    }
    if (attr_change->get_attr_name().compare("columns") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      generate_alter(table->columns(), list_change);
    } else if (attr_change->get_attr_name().compare("indices") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      // ignore changes if they're only reorderings
      if (list_change && list_change->subchanges()) {
        const grt::ListItemOrderChange *order_change = NULL;
        if (list_change->subchanges()->changes.size() == 1)
          order_change = dynamic_cast<const grt::ListItemOrderChange *>(list_change->subchanges()->begin()->get());
        if (!order_change || order_change->get_subchange()) {
          callback->alter_table_indexes_begin(table);
          generate_alter(table->indices(), list_change);
          callback->alter_table_indexes_end(table);
        }
      }
    } else if (attr_change->get_attr_name().compare("foreignKeys") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      // ignore changes if they're only reorderings
      if (list_change && list_change->subchanges()) {
        const grt::ListItemOrderChange *order_change = NULL;
        if (list_change->subchanges()->changes.size() == 1)
          order_change = dynamic_cast<const grt::ListItemOrderChange *>(list_change->subchanges()->begin()->get());
        if (!order_change || order_change->get_subchange()) {
          callback->alter_table_fks_begin(table);
          generate_alter(table->foreignKeys(), list_change);
          callback->alter_table_fks_end(table);
        }
      }
    } else if (attr_change->get_attr_name().compare("tableEngine") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      if (!str.empty())
        callback->alter_table_engine(table, str);
    } else if (attr_change->get_attr_name().compare("nextAutoInc") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_next_auto_inc(table, str);
    } else if (attr_change->get_attr_name().compare("password") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_password(table, str);
    } else if (attr_change->get_attr_name().compare("delayKeyWrite") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::IntegerRef n = grt::IntegerRef::cast_from(change->get_new_value());
      callback->alter_table_delay_key_write(table, n);
    } else if (attr_change->get_attr_name().compare("defaultCharacterSetName") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_charset(table, str);
    } else if (attr_change->get_attr_name().compare("defaultCollationName") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_collate(table, str);
    } else if (attr_change->get_attr_name().compare("mergeUnion") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_merge_union(table, str);
    } else if (attr_change->get_attr_name().compare("mergeInsert") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_merge_insert(table, str);
    }
// ALTER TABLE silently ignores these attributes
#if 0
    else if(attr_change->get_attr_name().compare("tableDataDir") == 0)
    {
      const grt::SimpleValueChange *change= static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str= grt::StringRef::cast_from(change->get_new_value());
      alter_sql.append("DATA DIRECTORY = '").append(str.c_str()).append("' ");
    }
    else if(attr_change->get_attr_name().compare("tableIndexDir") == 0)
    {
      const grt::SimpleValueChange *change= static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str= grt::StringRef::cast_from(change->get_new_value());
      alter_sql.append("INDEX DIRECTORY = '").append(str.c_str()).append("' ");
    }
#endif
    else if (attr_change->get_attr_name().compare("packKeys") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_pack_keys(table, str);
    } else if (attr_change->get_attr_name().compare("checksum") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::IntegerRef n = grt::IntegerRef::cast_from(change->get_new_value());
      callback->alter_table_checksum(table, n);
    } else if (attr_change->get_attr_name().compare("comment") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_comment(table, str);
    } else if (attr_change->get_attr_name().compare("rowFormat") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_row_format(table, str);
    } else if (attr_change->get_attr_name().compare("keyBlockSize") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_key_block_size(table, str);
    } else if (attr_change->get_attr_name().compare("avgRowLength") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_avg_row_length(table, str);
    } else if (attr_change->get_attr_name().compare("minRows") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_min_rows(table, str);
    } else if (attr_change->get_attr_name().compare("maxRows") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_max_rows(table, str);
    }
// to be added later, probably needs support for CREATE/ALTER SERVER
#if 0
    else if(attr_change->get_attr_name().compare("connection") == 0)
    {
    }
#endif
    else if (attr_change->get_attr_name().compare("connectionString") == 0) {
      const grt::SimpleValueChange *change =
        static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
      grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
      callback->alter_table_connection_string(table, str);
    } else if (!partitions_processed && ((attr_change->get_attr_name().compare("partitionType") == 0) ||
                                         (attr_change->get_attr_name().compare("partitionExpression") == 0) ||
                                         (attr_change->get_attr_name().compare("subpartitionType") == 0) ||
                                         (attr_change->get_attr_name().compare("subpartitionExpression") == 0) ||
                                         (attr_change->get_attr_name().compare("subpartitionCount") == 0))) {
      generate_set_partitioning(table, diffchange);
      partitions_processed = true;
    }
  }

  if (alter_table_flags & EverythingButForeignKeys) {
    bool is_range = (strcmp(table->partitionType().c_str(), "RANGE") == 0);

    // partitioning options that dont require PARTITION BY clause
    for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); (it != e) && !partitions_processed; it++) {
      const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());

      if (attr_change->get_attr_name().compare("partitionCount") == 0) {
        const grt::SimpleValueChange *change =
          static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
        grt::IntegerRef old_count = grt::IntegerRef::cast_from(change->get_old_value());

        // std::string part_count_sql(generate_change_partition_count(table, new_count));

        //// partition count alone can be changed only for HASH/KEY partitions
        //// generate_change_partition_count() will return empty string otherwise
        //// for RANGE/LIST we ignore change of this attribute and rely solely on
        //// partition definitions change
        std::string part_type(table->partitionType().c_str());
        if ((part_type.find("HASH") != std::string::npos) || (part_type.find("KEY") != std::string::npos)) {
          callback->alter_table_partition_count(table, old_count);
          partitions_processed = true;
        }
      } else if (attr_change->get_attr_name().compare("partitionDefinitions") == 0) {
        const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
        const grt::ChangeSet *part_cs = list_change->subchanges();
        for (grt::ChangeSet::const_iterator e2 = part_cs->end(), jt = part_cs->begin(); jt != e2; jt++) {
          const grt::DiffChange *part_change = jt->get();
          switch (part_change->get_change_type()) {
            case grt::ListItemAdded: {
              db_mysql_PartitionDefinitionRef part(db_mysql_PartitionDefinitionRef::cast_from(
                static_cast<const grt::ListItemAddedChange *>(part_change)->get_value()));
              callback->alter_table_add_partition(part, is_range);
            } break;
            case grt::ListItemRemoved: {
              db_mysql_PartitionDefinitionRef part = db_mysql_PartitionDefinitionRef::cast_from(
                static_cast<const grt::ListItemRemovedChange *>(part_change)->get_value());

              callback->alter_table_drop_partition(part->name().c_str());
            } break;
            case grt::ListItemModified: {
              db_mysql_PartitionDefinitionRef old_part(db_mysql_PartitionDefinitionRef::cast_from(
                static_cast<const grt::ListItemModifiedChange *>(part_change)->get_old_value()));
              db_mysql_PartitionDefinitionRef new_part(db_mysql_PartitionDefinitionRef::cast_from(
                static_cast<const grt::ListItemModifiedChange *>(part_change)->get_new_value()));

              callback->alter_table_reorganize_partition(old_part, new_part, is_range);
            } break;
            default:
              break;
          }

          partitions_processed = true;
        }
      }
    }
  }

  callback->alter_table_props_end(table);
}

void DiffSQLGeneratorBE::generate_alter_stmt(db_mysql_ViewRef old_view, db_mysql_ViewRef new_view,
                                             const grt::DiffChange *diffchange) {
  std::string view_name_for_filter(get_old_object_name_for_key(new_view, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_views.find(view_name_for_filter) == _filtered_views.end())
      return;

  generate_create_stmt(new_view);

  std::string new_view_name = _case_sensitive ? *new_view->name() : base::toupper(new_view->name());
  std::string old_view_name = _case_sensitive ? *old_view->name() : base::toupper(old_view->name());
  if (strcmp(new_view_name.c_str(), old_view_name.c_str())) // name changed - need to drop old view
    generate_drop_stmt(old_view);
}

void DiffSQLGeneratorBE::generate_routine_alter_stmt(db_mysql_RoutineRef old_routine, db_mysql_RoutineRef new_routine,
                                                     const grt::DiffChange *diffchange) {
  std::string routine_name_for_filter(get_old_object_name_for_key(new_routine, _case_sensitive));
  if (_use_filtered_lists)
    if (_filtered_routines.find(routine_name_for_filter) == _filtered_routines.end())
      return;

  if (new_routine == old_routine) {
    generate_drop_stmt(new_routine);
    generate_create_stmt(new_routine, true);
  } else {
    generate_drop_stmt(old_routine);
    generate_create_stmt(new_routine);
  }
}

void DiffSQLGeneratorBE::generate_alter_stmt(db_mysql_SchemaRef schema, const grt::DiffChange *diffchange) {
  bool process_alter_schema = true;
  std::string schema_name_for_filter(get_old_object_name_for_key(schema, _case_sensitive));

  if (_use_filtered_lists)
    if (_filtered_schemata.find(schema_name_for_filter) == _filtered_schemata.end())
      process_alter_schema = false;

  const grt::ChangeSet *cs = diffchange->subchanges();

  callback->alter_schema_props_begin(schema);

  if (process_alter_schema) {
    for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
      const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());

      if (attr_change->get_attr_name().compare("name") == 0) {
        const grt::SimpleValueChange *change =
          static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get());
        grt::StringRef str = grt::StringRef::cast_from(change->get_new_value());
        callback->alter_schema_name(schema, str);
      }
      if (attr_change->get_attr_name().compare("defaultCharacterSetName") == 0) {
        grt::ValueRef value =
          static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get())->get_new_value();
        callback->alter_schema_default_charset(schema, grt::StringRef::cast_from(value));
      } else if (attr_change->get_attr_name().compare("defaultCollationName") == 0) {
        grt::ValueRef value =
          static_cast<const grt::SimpleValueChange *>(attr_change->get_subchange().get())->get_new_value();
        callback->alter_schema_default_collate(schema, grt::StringRef::cast_from(value));
      }
    }
  }

  callback->alter_schema_props_end(schema);

  for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
    const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());
    if (attr_change->get_attr_name().compare("tables") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      const grt::ChangeSet *tables_cs = list_change->subchanges();
      for (grt::ChangeSet::const_iterator e2 = tables_cs->end(), jt = tables_cs->begin(); jt != e2; jt++) {
        const grt::DiffChange *table_change = jt->get();
        if (table_change->get_change_type() == grt::ListItemModified) {
          generate_alter_stmt_drops(
            db_mysql_TableRef::cast_from(
              static_cast<const grt::ListItemModifiedChange *>(table_change)->get_new_value()),
            static_cast<const grt::ListItemModifiedChange *>(table_change)->get_subchange().get());
        } else if (table_change->get_change_type() == grt::ListItemOrderChanged) {
          const grt::ListItemOrderChange *oc = static_cast<const grt::ListItemOrderChange *>(table_change);
          if (oc->get_subchange())
            generate_alter_stmt_drops(db_mysql_TableRef::cast_from(oc->get_subchange()->get_new_value()),
                                      oc->get_subchange()->get_subchange().get());
        }
      }
    }
  }

  for (grt::ChangeSet::const_iterator e = cs->end(), it = cs->begin(); it != e; it++) {
    const grt::ObjectAttrModifiedChange *attr_change = static_cast<const grt::ObjectAttrModifiedChange *>(it->get());
    if (attr_change->get_attr_name().compare("tables") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      const grt::ChangeSet *tables_cs = list_change->subchanges();

      // 1st pass, do everything except FKs
      for (grt::ChangeSet::const_iterator e2 = tables_cs->end(), jt = tables_cs->begin(); jt != e2; jt++) {
        const grt::DiffChange *table_change = jt->get();
        switch (table_change->get_change_type()) {
          case grt::ListItemAdded:
            generate_create_stmt(
              db_mysql_TableRef::cast_from(static_cast<const grt::ListItemAddedChange *>(table_change)->get_value()));
            break;
          case grt::ListItemRemoved:
            generate_drop_stmt(
              db_mysql_TableRef::cast_from(static_cast<const grt::ListItemRemovedChange *>(table_change)->get_value()));
            break;
          case grt::ListItemModified:
            generate_alter_stmt(
              db_mysql_TableRef::cast_from(
                static_cast<const grt::ListItemModifiedChange *>(table_change)->get_new_value()),
              static_cast<const grt::ListItemModifiedChange *>(table_change)->get_subchange().get(),
              _separate_foreign_keys ? EverythingButForeignKeys : Everything); // everything but FK 1st
            break;
          case grt::ListItemOrderChanged: {
            const grt::ListItemOrderChange *oc = static_cast<const grt::ListItemOrderChange *>(table_change);
            if (oc->get_subchange())
              generate_alter_stmt(db_mysql_TableRef::cast_from(oc->get_subchange()->get_new_value()),
                                  oc->get_subchange()->get_subchange().get(),
                                  _separate_foreign_keys ? EverythingButForeignKeys : Everything);
          } break;
          default:
            break;
        }
      }

      if (_separate_foreign_keys) {
        // 2nd pass, do FKs only
        for (grt::ChangeSet::const_iterator e2 = tables_cs->end(), jt = tables_cs->begin(); jt != e2; jt++) {
          const grt::DiffChange *table_change = jt->get();
          switch (table_change->get_change_type()) {
            case grt::ListItemAdded:
            case grt::ListItemRemoved:
              break;
            case grt::ListItemModified:
              generate_alter_stmt(db_mysql_TableRef::cast_from(
                                    static_cast<const grt::ListItemModifiedChange *>(table_change)->get_new_value()),
                                  static_cast<const grt::ListItemModifiedChange *>(table_change)->get_subchange().get(),
                                  OnlyForeignKeys); // FK only
              break;
            case grt::ListItemOrderChanged: {
              const grt::ListItemOrderChange *oc = static_cast<const grt::ListItemOrderChange *>(table_change);
              if (oc->get_subchange())
                generate_alter_stmt(db_mysql_TableRef::cast_from(oc->get_subchange()->get_new_value()),
                                    oc->get_subchange()->get_subchange().get(), OnlyForeignKeys);
            } break;
            default:
              break;
          }
        }
      }
    } else if (attr_change->get_attr_name().compare("views") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      const grt::ChangeSet *views_cs = list_change->subchanges();
      for (grt::ChangeSet::const_iterator e2 = views_cs->end(), jt = views_cs->begin(); jt != e2; jt++) {
        const grt::DiffChange *view_change = jt->get();
        switch (view_change->get_change_type()) {
          case grt::ListItemAdded: {
            db_mysql_ViewRef new_view =
              db_mysql_ViewRef::cast_from(static_cast<const grt::ListItemAddedChange *>(view_change)->get_value());
            generate_create_stmt(new_view);
          } break;
          case grt::ListItemRemoved: {
            generate_drop_stmt(
              db_mysql_ViewRef::cast_from(static_cast<const grt::ListItemRemovedChange *>(view_change)->get_value()));
          } break;
          case grt::ListItemModified: {
            db_mysql_ViewRef old_view = db_mysql_ViewRef::cast_from(
              grt::ValueRef(static_cast<const grt::ListItemModifiedChange *>(view_change)->get_old_value()));
            db_mysql_ViewRef new_view = db_mysql_ViewRef::cast_from(
              grt::ValueRef(static_cast<const grt::ListItemModifiedChange *>(view_change)->get_new_value()));
            generate_alter_stmt(old_view, new_view, view_change);
          } break;
          case grt::ListItemOrderChanged:
            // list item position change is not relevant but may hold diff inside
            {
              if (!static_cast<const grt::ListItemOrderChange *>(view_change)->get_subchange())
                break;
              db_mysql_ViewRef old_view = db_mysql_ViewRef::cast_from(grt::ValueRef(
                static_cast<const grt::ListItemOrderChange *>(view_change)->get_subchange()->get_old_value()));
              db_mysql_ViewRef new_view = db_mysql_ViewRef::cast_from(grt::ValueRef(
                static_cast<const grt::ListItemOrderChange *>(view_change)->get_subchange()->get_new_value()));

              generate_alter_stmt(old_view, new_view, view_change);
              break;
            }
          default:
            break;
        }
      }
    } else if (attr_change->get_attr_name().compare("routines") == 0) {
      const grt::MultiChange *list_change = static_cast<const grt::MultiChange *>(attr_change->get_subchange().get());
      const grt::ChangeSet *routines_cs = list_change->subchanges();
      for (grt::ChangeSet::const_iterator e2 = routines_cs->end(), jt = routines_cs->begin(); jt != e2; jt++) {
        const grt::DiffChange *routine_change = jt->get();
        switch (routine_change->get_change_type()) {
          case grt::ListItemAdded: {
            db_mysql_RoutineRef new_routine = db_mysql_RoutineRef::cast_from(
              static_cast<const grt::ListItemAddedChange *>(routine_change)->get_value());
            generate_create_stmt(new_routine);
          } break;
          case grt::ListItemRemoved: {
            generate_drop_stmt(db_mysql_RoutineRef::cast_from(
              static_cast<const grt::ListItemRemovedChange *>(routine_change)->get_value()));
          } break;
          case grt::ListItemModified: {
            db_mysql_RoutineRef new_routine = db_mysql_RoutineRef::cast_from(
              grt::ValueRef(static_cast<const grt::ListItemModifiedChange *>(routine_change)->get_new_value()));
            generate_routine_alter_stmt(new_routine, new_routine, routine_change);
          } break;
          case grt::ListItemOrderChanged:
            // list item position change is not relevant but may hold diff inside
            {
              if (!static_cast<const grt::ListItemOrderChange *>(routine_change)->get_subchange())
                break;
              db_mysql_RoutineRef old_routine = db_mysql_RoutineRef::cast_from(grt::ValueRef(
                static_cast<const grt::ListItemOrderChange *>(routine_change)->get_subchange()->get_old_value()));
              db_mysql_RoutineRef new_routine = db_mysql_RoutineRef::cast_from(grt::ValueRef(
                static_cast<const grt::ListItemOrderChange *>(routine_change)->get_subchange()->get_new_value()));
              generate_routine_alter_stmt(old_routine, new_routine, routine_change);
            }
            break;
          default:
            break;
        }
      }
    }
  }
}

void DiffSQLGeneratorBE::generate_alter_stmt(db_mysql_CatalogRef catalog, const grt::DiffChange *diffchange) {
  // process changes in schemata
  for (grt::ChangeSet::const_iterator e = diffchange->subchanges()->end(), it = diffchange->subchanges()->begin();
       it != e; it++) {
    const grt::DiffChange *subchange = it->get();
    if (subchange->get_change_type() == grt::ObjectAttrModified) {
      const grt::ObjectAttrModifiedChange *objattrchange =
        static_cast<const grt::ObjectAttrModifiedChange *>(subchange);

      if (objattrchange->get_attr_name().compare("schemata") == 0) {
        const grt::DiffChange *objattr_subchange = objattrchange->get_subchange().get();
        if (objattr_subchange->get_change_type() == grt::ListModified) {
          const grt::MultiChange *schemata_list_change = static_cast<const grt::MultiChange *>(objattr_subchange);

          for (grt::ChangeSet::const_iterator schemata_e = schemata_list_change->subchanges()->end(),
                                              schemata_it = schemata_list_change->subchanges()->begin();
               schemata_it != schemata_e; schemata_it++) {
            const grt::DiffChange *schema_subchange = schemata_it->get();
            switch (schema_subchange->get_change_type()) {
              case grt::ListItemAdded:
                generate_create_stmt(db_mysql_SchemaRef::cast_from(
                  static_cast<const grt::ListItemAddedChange *>(schema_subchange)->get_value()));
                break;
              case grt::ListItemRemoved:
                generate_drop_stmt(db_mysql_SchemaRef::cast_from(
                  static_cast<const grt::ListItemRemovedChange *>(schema_subchange)->get_value()));
                break;
              case grt::ListItemModified:
                generate_alter_stmt(
                  db_mysql_SchemaRef::cast_from(
                    static_cast<const grt::ListItemModifiedChange *>(schema_subchange)->get_new_value()),
                  static_cast<const grt::ListItemModifiedChange *>(schema_subchange)->get_subchange().get());
                break;
              case grt::ListItemOrderChanged: {
                const grt::ListItemOrderChange *oc = static_cast<const grt::ListItemOrderChange *>(schema_subchange);
                if (oc->get_subchange())
                  generate_alter_stmt(db_mysql_SchemaRef::cast_from(oc->get_subchange()->get_new_value()),
                                      oc->get_subchange()->get_subchange().get());
              } break;
              default:
                break;
            }
          }
        }
      }
    }
  }
}

static void fill_set_from_list(grt::StringListRef string_list, std::set<std::string> &string_set) {
  for (size_t count = string_list.count(), i = 0; i < count; i++)
    string_set.insert(std::string(string_list.get(i).c_str()));
}

DiffSQLGeneratorBE::DiffSQLGeneratorBE(grt::DictRef options, grt::DictRef dbtraits,
                                       DiffSQLGeneratorBEActionInterface *cb)
  : callback(cb),
    _gen_create_index(false),
    _use_filtered_lists(true),
    _skip_foreign_keys(false),
    _skip_fk_indexes(false),
    _case_sensitive(false),
    _use_oid_as_dict_key(false),
    _separate_foreign_keys(true) {
  if (!options.is_valid())
    return;
  _case_sensitive = (dbtraits.get_int("CaseSensitive", _case_sensitive) != 0);

  grt::StringListRef empty_list(grt::Initialized);
  _use_oid_as_dict_key = options.get_int("UseOIDAsResultDictKey", _use_oid_as_dict_key) != 0;
  _skip_foreign_keys = options.get_int("SkipForeignKeys", _skip_foreign_keys) != 0;
  _skip_fk_indexes = options.get_int("SkipFKIndexes", _skip_fk_indexes) != 0;
  _gen_create_index = (options.get_int("GenerateCreateIndex", _gen_create_index) != 0);
  _use_filtered_lists = options.get_int("UseFilteredLists", _use_filtered_lists) != 0;
  _separate_foreign_keys = options.get_int("SeparateForeignKeys", _separate_foreign_keys) != 0;
  cb->setOmitSchemas(options.get_int("OmitSchemas", 0) != 0);
  cb->set_gen_use(options.get_int("GenerateUse", 0) != 0);
  fill_set_from_list(grt::StringListRef::cast_from(options.get("UserFilterList", empty_list)), _filtered_users);
  fill_set_from_list(grt::StringListRef::cast_from(options.get("SchemaFilterList", empty_list)), _filtered_schemata);
  fill_set_from_list(grt::StringListRef::cast_from(options.get("TableFilterList", empty_list)), _filtered_tables);
  fill_set_from_list(grt::StringListRef::cast_from(options.get("ViewFilterList", empty_list)), _filtered_views);
  fill_set_from_list(grt::StringListRef::cast_from(options.get("RoutineFilterList", empty_list)), _filtered_routines);
  fill_set_from_list(grt::StringListRef::cast_from(options.get("TriggerFilterList", empty_list)), _filtered_triggers);
}

void DiffSQLGeneratorBE::process_diff_change(grt::ValueRef org_object, grt::DiffChange *diff, grt::DictRef map) {
  this->target_list = grt::StringListRef();
  this->target_map = map;

  do_process_diff_change(org_object, diff);
}

void DiffSQLGeneratorBE::process_diff_change(grt::ValueRef org_object, grt::DiffChange *diff, grt::StringListRef list,
                                             grt::ListRef<GrtNamedObject> objlist) {
  this->target_map = grt::DictRef();
  this->target_list = list;
  this->target_object_list = objlist;

  do_process_diff_change(org_object, diff);
}

void DiffSQLGeneratorBE::do_process_diff_change(grt::ValueRef org_object, grt::DiffChange *diff) {
  switch (diff->get_change_type()) {
    // case SimpleValue:
    case grt::ValueAdded:
      generate_create_stmt(db_mysql_CatalogRef::cast_from(dynamic_cast<grt::ValueAddedChange *>(diff)->get_value()));
      break;
    case grt::DictItemAdded:
      break;
    case grt::ListItemAdded:
      generate_create_stmt(db_mysql_CatalogRef::cast_from(dynamic_cast<grt::ListItemAddedChange *>(diff)->get_value()));
      break;

    case grt::ValueRemoved:
    case grt::ListItemRemoved:
    case grt::DictItemRemoved:
      generate_drop_stmt(db_mysql_CatalogRef::cast_from(org_object));
      break;

    case grt::ObjectModified:
    case grt::ObjectAttrModified:
    case grt::ListModified:
    case grt::ListItemModified:
    case grt::ListItemOrderChanged:
    case grt::DictModified:
    case grt::DictItemModified:
      generate_alter_stmt(db_mysql_CatalogRef::cast_from(org_object), diff);
      break;

    default:
      break;
  }
}
