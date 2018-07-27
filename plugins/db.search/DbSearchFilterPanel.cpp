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

#include "DbSearchFilterPanel.h"
#include <sstream>
#include <string>
#include "base/string_utilities.h"

void update_numeric(mforms::TextEntry& te) {
  long result = base::atoi<long>(te.get_string_value(), -1);
  if (result < 0)
    te.set_value("0");
}

DBSearchFilterPanel::DBSearchFilterPanel()
  : Box(false), _search_box(true), _filter_tree(mforms::TreeNoHeader), _limits_box(true) {
  set_spacing(12);

  _search_box.set_spacing(8);

  _search_text_label.set_text("Search for table fields that");
  _search_box.add(&_search_text_label, false, true);

  _filter_selector.add_item("CONTAINS");
  _filter_selector.add_item("Search using =");
  _filter_selector.add_item("Search using LIKE");
  _filter_selector.add_item("Search using REGEXP");

#ifdef _MSC_VER
  _filter_selector.set_size(150, -1);
#endif

  _filter_selector.set_selected(0);
  _search_box.add(&_filter_selector, false, true);

  _search_box.add(&_search_text, true, true);
  add(&_search_box, false, true);

  _limits_box.set_spacing(4);
  _limit_table_hint.set_text("Max. matches per table");
  _limit_table_hint.set_text_align(mforms::MiddleRight);
  _limits_box.add(&_limit_table_hint, false, true);
  _limits_box.add(&_limit_table, false, true);
  _limit_table.set_size(80, -1);
  _limit_table.set_value("100");
  _limit_table.signal_changed()->connect(std::bind(update_numeric, std::ref(_limit_table)));
  _limit_total_hint.set_text("Max. total matches");
  _limit_total_hint.set_text_align(mforms::MiddleRight);
  _limit_total.set_size(80, -1);
  _limits_box.add(&_limit_total_hint, false, true);
  _limits_box.add(&_limit_total, false, true);
  _limit_total.signal_changed()->connect(std::bind(update_numeric, std::ref(_limit_total)));
  _limit_total.set_value("100000");

  _search_all_type_check.set_text("Search columns of all types");
  _search_all_type_check.set_tooltip(
    "If checked, non-text type columns will be casted to CHAR to match. Otherwise, only text type (CHAR, VARCHAR, "
    "TEXT) will be searched.");
  _limits_box.add(&_search_all_type_check, false, true);

  _search_button.set_text("Start Search");
  _search_button.set_size(120, -1);
  _limits_box.add(&_search_button, false, true);
  add(&_limits_box, false, true);
  //  add(&_search_all_type_check, false, true);
  //  _exclude_check.set_text("Invert table selection (search all tables except selected)");
  //  add(&_exclude_check, false, true);

  _filter_tree.add_column(mforms::StringColumnType, "", 150, true);
  _filter_tree.end_columns();
  _filter_tree.set_cell_edit_handler(std::bind(&DBSearchFilterPanel::cell_edited, this, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3));
  _filter_tree.add_node()->set_string(0, "Schema.Table.Column");
  _hint_label.set_text(
    "Place list of patterns in the form of schema.table.[column].\nYou can use % or _ as wildcarts.");
  //  _table.add(&_filter_tree, 1, 2, 3, 4, mforms::FillAndExpand);
}

void DBSearchFilterPanel::cell_edited(mforms::TreeNodeRef node, int column, const std::string& value) {
  if ((_filter_tree.count() > 1) && (value == ""))
    node->remove_from_parent();

  if (column == 0) {
    node->set_string(0, value);
    if (_filter_tree.row_for_node(node) + 1 == _filter_tree.count())
      _filter_tree.add_node()->set_string(0, "Schema.Table.Column");
  }
}

void DBSearchFilterPanel::set_searching(bool flag) {
  _search_text.set_enabled(!flag);
  _search_all_type_check.set_enabled(!flag);
  _exclude_check.set_enabled(!flag);
  _filter_selector.set_enabled(!flag);
  _limit_table.set_enabled(!flag);
  _limit_total.set_enabled(!flag);

  if (flag)
    _search_button.set_text("Stop");
  else
    _search_button.set_text("Start Search");
}
