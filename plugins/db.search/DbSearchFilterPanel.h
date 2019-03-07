/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DB_SEARCH_FILTER_PANEL_H_
#define _DB_SEARCH_FILTER_PANEL_H_
#include "mforms/mforms.h"

class DBSearchFilterPanel : public mforms::Box {
private:
  mforms::Box _search_box;
  mforms::Label _search_text_label;
  mforms::TextEntry _search_text;
  mforms::CheckBox _exclude_check;
  mforms::CheckBox _search_all_type_check;
  mforms::Selector _filter_selector;
  mforms::TreeView _filter_tree;
  mforms::Label _hint_label;
  mforms::Box _limits_box;
  mforms::Label _limit_table_hint;
  mforms::TextEntry _limit_table;
  mforms::Label _limit_total_hint;
  mforms::TextEntry _limit_total;
  mforms::Button _search_button;

public:
  DBSearchFilterPanel();
  void cell_edited(mforms::TreeNodeRef node, int column, const std::string &value);

  int get_limit_table() {
    return atoi(_limit_table.get_string_value().c_str());
  }

  void set_limit_table(const std::string &i) {
    _limit_table.set_value(i);
  }

  int get_limit_total() {
    return atoi(_limit_total.get_string_value().c_str());
  }

  void set_limit_total(const std::string &i) {
    _limit_total.set_value(i);
  }

  bool search_all_types() {
    return _search_all_type_check.get_active();
  }

  int get_search_type() {
    return _filter_selector.get_selected_index();
  }

  void set_search_type(int i) {
    _filter_selector.set_selected(i);
  }

  bool exclude() {
    return _exclude_check.get_active();
  }

  void set_exclude(bool flag) {
    _exclude_check.set_active(flag);
  }

  std::string get_search_text() {
    return _search_text.get_string_value();
  }

  template <typename string_list_ref_t>
  void get_filters(string_list_ref_t &result) {
    for (int i = 0; i < _filter_tree.count() - 1; ++i)
      result.insert(_filter_tree.node_at_row(i)->get_string(0));
  }

  void set_searching(bool flag);

  mforms::Button *search_button() {
    return &_search_button;
  }
  mforms::TextEntry *search_field() {
    return &_search_text;
  }
};

#endif //#ifndef _DB_SEARCH_FILTER_PANEL_H_
