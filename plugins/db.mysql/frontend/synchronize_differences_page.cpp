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

#include "synchronize_differences_page.h"
#include "base/log.h"
DEFAULT_LOG_DOMAIN("Synchronize")

SynchronizeDifferencesPage::SynchronizeDifferencesPage(grtui::WizardForm *form,
                                                       SynchronizeDifferencesPageBEInterface *be)
  : WizardPage(form, "diffs"),
    _be(be),
    _tree(mforms::TreeDefault),
    _diff_sql_text(),
    _splitter(false),
    _bottom_box(true) {
  set_title(_("Choose Direction to Apply Changes"));
  set_short_title(_("Select Changes to Apply"));

  _heading.set_wrap_text(true);
  _heading.set_text(
    _("Double click arrows in the list to choose whether to ignore changes, update "
      "the model with database changes or vice-versa. You can also apply an action to multiple selected rows."));
  add(&_heading, false, true);
  add(&_splitter, true, true);

  _splitter.add(&_tree);
  _tree.set_selection_mode(mforms::TreeSelectMultiple);
  _diff_sql_text.set_features(mforms::FeatureReadOnly | mforms::FeatureWrapText, true);
  _diff_sql_text.set_features(mforms::FeatureGutter, false);
  _diff_sql_text.set_language(mforms::LanguageMySQL);
  _splitter.add(&_diff_sql_text);

  add(&_bottom_box, false, true);

  _bottom_box.set_spacing(12);

  _update_model.set_text(_("Update Model"));
  _update_model.set_tooltip(_("Update the model with changes detected in the target database/script."));
  _skip.set_text(_("Ignore"));
  _skip.set_tooltip(_("Ignore the change and do not update neither the database/script nor the model."));
  _update_source.set_text(_("Update Source"));
  _update_source.set_tooltip(_("Update the database/script with changes detected in the source model."));

  _edit_table_mapping.set_text(_("Table Mapping..."));
  _edit_table_mapping.set_tooltip(
    _("Fix table mapping, in case tables were renamed but are not being correctly recognized as such."));

  _edit_column_mapping.set_text(_("Column Mapping..."));
  _edit_column_mapping.set_tooltip(
    _("Fix column mapping, in case columns were renamed but are not being correctly recognized as such."));

  _bottom_box.add(&_update_model, false, true);
  _bottom_box.add(&_skip, false, true);
  _bottom_box.add(&_update_source, false, true);
  _bottom_box.add_end(&_edit_column_mapping, false, true);
  _bottom_box.add_end(&_edit_table_mapping, false, true);

  scoped_connect(_update_source.signal_clicked(), std::bind(&SynchronizeDifferencesPage::update_source, this));
  scoped_connect(_update_model.signal_clicked(), std::bind(&SynchronizeDifferencesPage::update_model, this));
  scoped_connect(_skip.signal_clicked(), std::bind(&SynchronizeDifferencesPage::update_none, this));
  scoped_connect(_edit_table_mapping.signal_clicked(),
                 std::bind(&SynchronizeDifferencesPage::edit_table_mapping, this));
  scoped_connect(_edit_column_mapping.signal_clicked(),
                 std::bind(&SynchronizeDifferencesPage::edit_column_mapping, this));

  _tree.add_column(mforms::IconStringColumnType, _be->get_col_name(0), 200, false);
  _tree.add_column(mforms::IconStringColumnType, _be->get_col_name(1), 50, false);
  _tree.add_column(mforms::IconStringColumnType, _be->get_col_name(2), 200, false);
  _tree.end_columns();

  scoped_connect(_tree.signal_node_activated(), std::bind(&SynchronizeDifferencesPage::activate_node, this,
                                                          std::placeholders::_1, std::placeholders::_2));
  scoped_connect(_tree.signal_changed(), std::bind(&SynchronizeDifferencesPage::select_row, this));
}

SynchronizeDifferencesPage::~SynchronizeDifferencesPage() {
  /* we don't know where do these come from, so we can't dispose of them here
  if (get_source_catalog && _src.is_valid())
    _src->reset_references();
  if (get_target_catalog && _dst.is_valid())
    _dst->reset_references();
   */
}

void SynchronizeDifferencesPage::update_original_columns(std::list<db_ColumnRef> &changed_columns) {
  // maps from obejct id of copy to the original one
  std::map<std::string, db_SchemaRef> schema_map;
  std::map<std::string, db_TableRef> table_map;

  for (std::list<db_ColumnRef>::const_iterator col = changed_columns.begin(); col != changed_columns.end(); ++col) {
    db_TableRef table = db_TableRef::cast_from((*col)->owner());
    db_SchemaRef schema = db_SchemaRef::cast_from(table->owner());

    db_TableRef orig_table;

    if (table_map.find(table.id()) != table_map.end())
      orig_table = table_map[table.id()];
    else {
      db_SchemaRef orig_schema;
      if (schema_map.find(schema.id()) != schema_map.end())
        orig_schema = schema_map[schema.id()];
      else {
        orig_schema = grt::find_named_object_in_list(_src->schemata(), schema->name());
        if (orig_schema.is_valid())
          schema_map[schema.id()] = orig_schema;
      }
      if (orig_schema.is_valid()) {
        orig_table = grt::find_named_object_in_list(orig_schema->tables(), table->name());
        if (orig_table.is_valid())
          table_map[table.id()] = orig_table;
      }
    }
    if (!orig_table.is_valid()) {
      logError("Internal error, could not find original object for table %s.%s\n", schema->name().c_str(),
               table->name().c_str());
      continue;
    }

    db_ColumnRef orig_column = grt::find_named_object_in_list(orig_table->columns(), (*col)->name());
    if (orig_column.is_valid())
      orig_column->oldName((*col)->oldName());
    else
      logError("Could not find original column for %s [old %s]\n", (*col)->name().c_str(), (*col)->oldName().c_str());
  }
}

void SynchronizeDifferencesPage::edit_column_mapping() {
  mforms::TreeNodeRef node;
  db_TableRef left, right;
  if ((node = _tree.get_selected_node())) {
    bec::NodeId n(node->get_tag());

    right = db_TableRef::cast_from(_be->get_db_object(n));
    left = db_TableRef::cast_from(_be->get_model_object(n));

#if 0
    // some debug helper stuff...
#warning debug helper code is enabled

    log_info("LEFT\n");
    grt::dump_value(left->indices());
    log_info("\nRIGHT\n");
    grt::dump_value(right->indices());
    return;
#endif

    ColumnNameMappingEditor editor(wizard(), _be, left, right);
    std::list<db_ColumnRef> changed_columns;
    if (editor.run(changed_columns)) {
      // the diff tree code creates copies of the original catalogs to create the diff, so
      // we need to update the mapping changes in the original catalogs for them to take effect
      update_original_columns(changed_columns);
      pre_load();
    }
  }
}

void SynchronizeDifferencesPage::update_original_tables(std::list<db_TableRef> &changed_tables) {
  for (std::list<db_TableRef>::const_iterator tbl = changed_tables.begin(); tbl != changed_tables.end(); ++tbl) {
    db_SchemaRef orig_schema = grt::find_named_object_in_list(_src->schemata(), (*tbl)->owner()->name());
    if (!orig_schema.is_valid()) {
      logError("Could not find original schema for %s\n", (*tbl)->owner()->name().c_str());
      continue;
    }
    db_TableRef orig_table = grt::find_named_object_in_list(orig_schema->tables(), (*tbl)->name());
    if (orig_table.is_valid())
      orig_table->oldName((*tbl)->oldName());
    else
      logError("Could not find original table for %s\n", (*tbl)->name().c_str());
  }
}

void SynchronizeDifferencesPage::edit_table_mapping() {
  mforms::TreeNodeRef node;
  db_SchemaRef left, right;
  if ((node = _tree.get_selected_node())) {
    bec::NodeId n(node->get_tag());

    left = db_SchemaRef::cast_from(_be->get_model_object(n.parent()));
    right = db_SchemaRef::cast_from(_be->get_db_object(n.parent()));

    TableNameMappingEditor editor(wizard(), _be, left, right);
    std::list<db_TableRef> changed_tables;
    if (editor.run(changed_tables)) {
      // the diff tree code creates copies of the original catalogs to create the diff, so
      // we need to update the mapping changes in the original catalogs for them to take effect
      update_original_tables(changed_tables);
      pre_load();
    }
  }
}

void SynchronizeDifferencesPage::select_row() {
  mforms::TreeNodeRef node;
  std::string sql;
  if ((node = _tree.get_selected_node())) {
    bec::NodeId n(node->get_tag());

    grt::ValueRef obj = _be->get_db_object(n);
    grt::ValueRef obj2 = _be->get_model_object(n);

    switch (_be->get_apply_direction(n)) {
      case DiffNode::ApplyToDb:
        if (GrtNamedObjectRef::can_wrap(obj))
          sql.append(_be->get_sql_for_object(GrtNamedObjectRef::cast_from(obj)));
        if (GrtNamedObjectRef::can_wrap(obj2))
          sql.append(_be->get_sql_for_object(GrtNamedObjectRef::cast_from(obj2)));
        break;

      case DiffNode::DontApply:
      case DiffNode::CantApply:
        break;

      case DiffNode::ApplyToModel:
        sql = "Update Source";
        break;
    }

    _edit_column_mapping.set_enabled(obj.is_valid() && obj2.is_valid() && db_TableRef::can_wrap(obj));

    if (n.depth() > 1) {
      if (_be->get_db_object(n.parent()).is_valid()) // make sure target schema exists
        _edit_table_mapping.set_enabled(obj2.is_valid() && db_TableRef::can_wrap(obj2));
      else
        _edit_table_mapping.set_enabled(false);
    } else
      _edit_table_mapping.set_enabled(false);
  } else {
    _edit_table_mapping.set_enabled(false);
    _edit_column_mapping.set_enabled(false);
  }
  _diff_sql_text.set_features(mforms::FeatureReadOnly, false);
  _diff_sql_text.set_value(sql);
  _diff_sql_text.set_features(mforms::FeatureReadOnly, true);
}

void SynchronizeDifferencesPage::activate_node(mforms::TreeNodeRef node, int column) {
  if (column == 1) {
    bec::NodeId n(node->get_tag());
    _be->set_next_apply_direction(n);

    refresh_node(node);
    select_row();
  }
}

void SynchronizeDifferencesPage::refresh_node(mforms::TreeNodeRef node) {
  bec::NodeId n(node->get_tag());
  node->set_icon_path(0, get_icon_path(_diff_tree->get_field_icon(n, DiffTreeBE::ModelObjectName, bec::Icon16)));
  node->set_icon_path(1, get_icon_path(_diff_tree->get_field_icon(n, DiffTreeBE::ApplyDirection, bec::Icon16)));
  node->set_icon_path(2, get_icon_path(_diff_tree->get_field_icon(n, DiffTreeBE::DbObjectName, bec::Icon16)));

  for (int i = 0; i < node->count(); i++)
    refresh_node(node->get_child(i));
}

void SynchronizeDifferencesPage::set_catalog_getter_slot(const std::function<db_CatalogRef()> &source_catalog_slot,
                                                         const std::function<db_CatalogRef()> &target_catalog_slot) {
  get_source_catalog = source_catalog_slot;
  get_target_catalog = target_catalog_slot;
}

void SynchronizeDifferencesPage::set_src(const db_CatalogRef cat) {
  _src = cat;
}

void SynchronizeDifferencesPage::set_dst(const db_CatalogRef cat) {
  _dst = cat;
}

std::string SynchronizeDifferencesPage::get_icon_path(bec::IconId icon) {
  if (_icons.find(icon) == _icons.end()) {
    std::string p = bec::IconManager::get_instance()->get_icon_file(icon);
    _icons[icon] = p;
    return p;
  }
  return _icons[icon];
}

void SynchronizeDifferencesPage::load_model(std::shared_ptr<DiffTreeBE> model, bec::NodeId node,
                                            mforms::TreeNodeRef tnode) {
  for (size_t c = model->count_children(node), i = 0; i < c; i++) {
    std::string value;
    mforms::TreeNodeRef child = tnode->add_child();
    bec::NodeId tmp(bec::NodeId(node).append(i));

    model->get_field(tmp, DiffTreeBE::ModelObjectName, value);
    child->set_string(0, value);
    model->get_field(tmp, DiffTreeBE::DbObjectName, value);
    child->set_string(2, value);
    child->set_tag(tmp.toString());
    refresh_node(child);
    load_model(model, tmp, child);
  }
}

bool SynchronizeDifferencesPage::pre_load() {
  grt::StringListRef schemas_to_skip(grt::StringListRef::cast_from(values().get("unSelectedSchemata")));
  if (get_source_catalog)
    _src = get_source_catalog();
  if (get_target_catalog)
    _dst = get_target_catalog();

  _diff_tree = _be->init_diff_tree(std::vector<std::string>(), _src, _dst, schemas_to_skip, values());

  _tree.freeze_refresh();
  _tree.clear();
  mforms::TreeNodeRef root = _tree.root_node();
  load_model(_diff_tree, bec::NodeId(), root);
  _tree.thaw_refresh();

  if (_tree.count() > 0) {
    // Expand all nodes that have modified children.
    for (size_t i = 0; i < _diff_tree->count(); ++i) {
      bec::NodeId schema(i);
      mforms::TreeNodeRef schema_node = root->get_child((int)i);
      for (size_t j = 0; j < _diff_tree->count_children(schema); ++j) {
        bec::NodeId object(_diff_tree->get_child(schema, j));

        if (_diff_tree->get_apply_direction(_diff_tree->get_child(schema, j)) != DiffNode::CantApply)
          schema_node->expand();

        // Expand the object node if it contains sub nodes with changes.
        mforms::TreeNodeRef object_node = schema_node->get_child((int)j);
        for (size_t k = 0; k < _diff_tree->count_children(object); ++k) {
          if (_diff_tree->get_apply_direction(_diff_tree->get_child(object, k)) != DiffNode::CantApply) {
            object_node->expand();
            break;
          }
        }
      }
    }
  }
#ifdef __linux__
  _splitter.set_divider_position(_splitter.get_height() == 1 ? 200 : _splitter.get_height() * 2 / 3);
#else
  _splitter.set_divider_position(_splitter.get_height() * 2 / 3);
#endif

  select_row();

  return true;
}

void SynchronizeDifferencesPage::update_source() {
  std::list<mforms::TreeNodeRef> nodes;
  if (!(nodes = _tree.get_selection()).empty()) {
    for (std::list<mforms::TreeNodeRef>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
      bec::NodeId n((*iter)->get_tag());
      _be->set_apply_direction(n, DiffNode::ApplyToDb, true);
      refresh_node(*iter);
    }
  }
  select_row();
}

void SynchronizeDifferencesPage::update_model() {
  std::list<mforms::TreeNodeRef> nodes;
  if (!(nodes = _tree.get_selection()).empty()) {
    for (std::list<mforms::TreeNodeRef>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
      bec::NodeId n((*iter)->get_tag());
      _be->set_apply_direction(n, DiffNode::ApplyToModel, true);
      refresh_node(*iter);
    }
  }
  select_row();
}

void SynchronizeDifferencesPage::update_none() {
  std::list<mforms::TreeNodeRef> nodes;
  if (!(nodes = _tree.get_selection()).empty()) {
    for (std::list<mforms::TreeNodeRef>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
      bec::NodeId n((*iter)->get_tag());
      _be->set_apply_direction(n, DiffNode::DontApply, true);
      refresh_node(*iter);
    }
  }
  select_row();
}
