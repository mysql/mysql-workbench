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

#include "db_mysql_sql_script_sync.h"

#include "mforms/treeview.h"
#include "mforms/splitter.h"
#include "mforms/label.h"
#include "mforms/code_editor.h"
#include "mforms/box.h"
#include "mforms/button.h"

#include "grtui/grt_wizard_plugin.h"
#include "name_mapping_editor.h"

class SynchronizeDifferencesPage : public grtui::WizardPage {
public:
  SynchronizeDifferencesPage(grtui::WizardForm *form, SynchronizeDifferencesPageBEInterface *be);
  virtual ~SynchronizeDifferencesPage();
  void update_original_tables(std::list<db_TableRef> &changed_tables);
  void update_original_columns(std::list<db_ColumnRef> &changed_columns);
  void edit_table_mapping();
  void edit_column_mapping();
  void select_row();
  void activate_node(mforms::TreeNodeRef node, int column);
  void set_catalog_getter_slot(const std::function<db_CatalogRef()> &source_catalog_slot,
                               const std::function<db_CatalogRef()> &target_catalog_slot);

  void set_src(const db_CatalogRef cat);
  void set_dst(const db_CatalogRef cat);

  std::string get_icon_path(bec::IconId icon);
  void load_model(std::shared_ptr<DiffTreeBE> model, bec::NodeId node, mforms::TreeNodeRef tnode);
  virtual bool pre_load();

  void update_source();
  void update_model();
  void update_none();

  //  virtual void extra_clicked();
  //  virtual std::string extra_button_caption();

protected:
  void refresh_node(mforms::TreeNodeRef node);
  //  bool node_has_changes(std::shared_ptr<DiffTreeBE> model, bec::NodeId);

  SynchronizeDifferencesPageBEInterface *_be;
  std::function<db_CatalogRef()> get_source_catalog;
  std::function<db_CatalogRef()> get_target_catalog;
  db_CatalogRef _src, _dst;

  std::map<bec::IconId, std::string> _icons;

  mforms::TreeView _tree;
  std::shared_ptr<DiffTreeBE> _diff_tree;
  mforms::Label _heading;
  ::mforms::CodeEditor _diff_sql_text;
  ::mforms::Splitter _splitter;

  mforms::Box _bottom_box;
  mforms::Button _select_all;
  mforms::Button _select_children;
  mforms::Button _update_source;
  mforms::Button _update_model;
  mforms::Button _skip;
  mforms::Button _edit_table_mapping;
  mforms::Button _edit_column_mapping;

  bool _hide_unchanged;
};
