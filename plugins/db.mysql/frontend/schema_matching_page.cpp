/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "schema_matching_page.h"
#include "grt/icon_manager.h"
#include "grts/structs.db.h"
#include "mforms/selector.h"

#include "db_plugin_be.h"

class SchemaMatchingPage::OverridePanel : public mforms::Box {
public:
  OverridePanel() : mforms::Box(true) {
    set_spacing(8);
    _button.set_text("Override Target");
    _button.signal_clicked()->connect(std::bind(&OverridePanel::override, this));
    add(mforms::manage(new mforms::Label("Override target schema to be synchronized with:")), false, true);
    add(&_selector, true, true);
    add(&_button, false, true);
  }

  void override() {
    std::string s = _selector.get_string_value();
    _node->set_string(2, s);
    _node->set_string(3, "overriden");
  }

  void set_schemas(const std::list<std::string> &schema_names) {
    _selector.add_items(schema_names);
  }

  void set_active(mforms::TreeNodeRef node) {
    _node = node;
    _selector.set_value(node->get_string(2));
  }

private:
  mforms::TreeNodeRef _node;
  mforms::Selector _selector;
  mforms::Button _button;
};

static void select_all(mforms::TreeView *tree, SchemaMatchingPage *page) {
  for (int i = 0; i < tree->count(); i++)
    tree->node_at_row(i)->set_bool(0, true);
  page->validate();
}

static void unselect_all(mforms::TreeView *tree, SchemaMatchingPage *page) {
  for (int i = 0; i < tree->count(); i++)
    tree->node_at_row(i)->set_bool(0, false);
  page->validate();
}

SchemaMatchingPage::SchemaMatchingPage(grtui::WizardForm *form, const char *name, const std::string &left_name,
                                       const std::string &right_name, bool unselect_by_default)
  : WizardPage(form, name), _header(true), _tree(mforms::TreeFlatList), _unselect_by_default(unselect_by_default) {
  _header.set_spacing(4);

  _image.set_image(bec::IconManager::get_instance()->get_icon_path("db.Schema.32x32.png"));
  _header.add(&_image, false);

  _label.set_text_align(mforms::MiddleLeft);
  _label.set_text(_("Select the Schemata to be Synchronized:"));
  _label.set_style(mforms::BoldStyle);
  _header.add(&_label, true, true);

  add(&_header, false, true);

  set_short_title(_("Select Schemas"));
  set_title(_("Select the Schemas to be Synchronized"));

  _menu.add_item_with_title("Select All", std::bind(select_all, &_tree, this), "", "");
  _menu.add_item_with_title("Unselect All", std::bind(unselect_all, &_tree, this), "", "");
#ifdef __linux__
  _tree.add_column(mforms::CheckColumnType, "", 40, true);
#else
  _tree.add_column(mforms::CheckColumnType, "", 30, true);
#endif
  _tree.add_column(mforms::IconStringColumnType, left_name, 150, false);
  _tree.add_column(mforms::StringColumnType, right_name, 150, false);
  _tree.add_column(mforms::IconStringColumnType, "", 300, false);
  _tree.end_columns();
  _tree.set_context_menu(&_menu);
  _tree.set_cell_edit_handler(std::bind(&SchemaMatchingPage::cell_edited, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));
  scoped_connect(_tree.signal_changed(), std::bind(&SchemaMatchingPage::selection_changed, this));

  add(&_tree, true, true);

  _override = mforms::manage(new OverridePanel());
  add(_override, false, true);

  add(&_missing_label, false, true);

  _missing_label.show(false);
  _missing_label.set_style(mforms::SmallHelpTextStyle);
}

void SchemaMatchingPage::cell_edited(mforms::TreeNodeRef node, int column, const std::string &value) {
  if (column == 0) {
    node->set_bool(column, value != "0");
    validate();
  }
}

bool SchemaMatchingPage::allow_next() {
  int c = _tree.count();
  for (int i = 0; i < c; i++) {
    mforms::TreeNodeRef node(_tree.root_node()->get_child(i));
    if (node->get_bool(0))
      return true;
  }
  return false;
}

void SchemaMatchingPage::leave(bool advancing) {
  if (advancing) {
    grt::StringListRef unlist(grt::Initialized);
    grt::StringListRef list(grt::Initialized);
    grt::StringListRef orig_list(grt::Initialized);

    int c = _tree.count();
    for (int i = 0; i < c; i++) {
      mforms::TreeNodeRef node(_tree.node_at_row(i));
      if (node->get_bool(0)) {
        list.insert(node->get_string(2));
        orig_list.insert(node->get_string(1));
      } else
        unlist.insert(node->get_string(2));
    }
    values().set("unSelectedSchemata", unlist);
    values().set("selectedSchemata", list);
    values().set("selectedOriginalSchemata", orig_list);
  }
  WizardPage::leave(advancing);
}

std::map<std::string, std::string> SchemaMatchingPage::get_mapping() {
  std::map<std::string, std::string> mapping;
  int c = _tree.count();
  for (int i = 0; i < c; i++) {
    mforms::TreeNodeRef node(_tree.node_at_row(i));
    if (node->get_bool(0)) {
      if (node->get_string(1) != node->get_string(2) && !node->get_string(2).empty())
        mapping[node->get_string(1)] = node->get_string(2);
    }
  }
  return mapping;
}

void SchemaMatchingPage::enter(bool advancing) {
  if (advancing) {
    int missing = 0;
    _tree.clear();

    {
      grt::IntegerRef server_case_sensitive(grt::IntegerRef::cast_from(values().get("server_is_case_sensitive")));
      // list of schemas from source (usually model).. must be filled by caller before this is reached
      grt::StringListRef db_list(grt::StringListRef::cast_from(values().get("schemata")));
      // list of schemas from target (usually DB or script).. must be filled by caller before this is reached
      grt::StringListRef target_db_list(grt::StringListRef::cast_from(values().get("targetSchemata")));

      std::list<std::string> db_schema_names;
      for (grt::StringListRef::const_iterator j = target_db_list.begin(); j != target_db_list.end(); ++j)
        db_schema_names.push_back(*j);
      db_schema_names.sort(std::bind(base::same_string, std::placeholders::_1, std::placeholders::_2, true));

      _override->set_schemas(db_schema_names);

      std::vector<std::string> sorted_names;
      for (grt::StringListRef::const_iterator sname = db_list.begin(); sname != db_list.end(); ++sname)
        sorted_names.push_back(*sname);
      std::sort(sorted_names.begin(), sorted_names.end(),
                std::bind(base::same_string, std::placeholders::_1, std::placeholders::_2, true));

      // check for schemas that exist only in the model and not in DB
      for (std::vector<std::string>::const_iterator sname = sorted_names.begin(); sname != sorted_names.end();
           ++sname) {
        mforms::TreeNodeRef node = _tree.add_node();

        std::string target_name;
        bool found_name = false;

        node->set_icon_path(1, "db.Schema.16x16.png");
        node->set_string(1, *sname);

        // check if the target has the list of schemas we want
        for (grt::StringListRef::const_iterator j = target_db_list.begin(); j != target_db_list.end(); ++j) {
          if (base::same_string(*j, *sname, server_case_sensitive == 1)) {
            found_name = true;
            target_name = *j;
          }
        }

        if (!found_name) {
          node->set_bool(0, false);
          node->set_string(2, *sname);
          node->set_string(3, _("schema not found in target"));
          missing++;
        } else {
          if (!_unselect_by_default)
            node->set_bool(0, true);
          node->set_string(2, target_name);
        }
      }
    }

    if (missing > 0) {
      _missing_label.set_text(
        _("The schemata from your model are missing from the target.\nIf you are creating them for the first time use "
          "the Forward Engineer function."));
      _missing_label.show(true);
    }

    selection_changed();
  }
}

void SchemaMatchingPage::selection_changed() {
  mforms::TreeNodeRef sel(_tree.get_selected_node());
  if (sel) {
    _override->set_enabled(true);
    _override->set_active(sel);
  } else
    _override->set_enabled(false);
}
