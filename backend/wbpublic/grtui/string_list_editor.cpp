/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/common.h"
#include "string_list_editor.h"
#include "base/string_utilities.h"

using namespace grtui;

StringListEditor::StringListEditor(mforms::Form *owner, const bool reorderable)
  : mforms::Form(owner, mforms::FormResizable),
    _vbox(false),
    _tree(mforms::TreeFlatList | (reorderable ? mforms::TreeAllowReorderRows : (mforms::TreeOptions)0)),
    _button_box(true) {
  set_name("list_editor");
  _tree.add_column(mforms::StringColumnType, "Value", 300, true);
  _tree.end_columns();

  set_content(&_vbox);
  _vbox.set_padding(12);
  _vbox.set_spacing(12);
  _button_box.set_spacing(12);

  _vbox.add(&_tree, true, true);
  _vbox.add(&_button_box, false, true);

  _ok_button.set_text(_("OK"));
  _cancel_button.set_text(_("Cancel"));
  _ok_button.enable_internal_padding(true);
  _cancel_button.enable_internal_padding(true);

  _add_button.set_text(_("Add"));
  _del_button.set_text(_("Delete"));
  _add_button.enable_internal_padding(true);
  _del_button.enable_internal_padding(true);

  _button_box.add(&_add_button, false, true);
  _button_box.add(&_del_button, false, true);

  _button_box.add_end(&_ok_button, false, true);
  _button_box.add_end(&_cancel_button, false, true);

  scoped_connect(_add_button.signal_clicked(), std::bind(&StringListEditor::add, this));
  scoped_connect(_del_button.signal_clicked(), std::bind(&StringListEditor::del, this));

  set_size(400, 320);
}

void StringListEditor::add() {
  _tree.select_node(_tree.add_node());
}

void StringListEditor::del() {
  mforms::TreeNodeRef node = _tree.get_selected_node();
  if (node)
    node->remove_from_parent();
}

bool StringListEditor::run() {
  return run_modal(&_ok_button, &_cancel_button);
}

void StringListEditor::set_string_list(const std::vector<std::string> &strings) {
  _tree.clear();
  for (std::vector<std::string>::const_iterator r = strings.begin(); r != strings.end(); ++r) {
    _tree.add_node()->set_string(0, *r);
  }
}

void StringListEditor::set_grt_string_list(const grt::StringListRef &strings) {
  _tree.clear();
  for (grt::StringListRef::const_iterator r = strings.begin(); r != strings.end(); ++r) {
    _tree.add_node()->set_string(0, (*r).c_str());
  }
}

std::vector<std::string> StringListEditor::get_string_list() {
  std::vector<std::string> list;

  for (int c = _tree.count(), i = 0; i < c; i++) {
    list.push_back(_tree.root_node()->get_child(i)->get_string(0));
  }

  return list;
}

grt::StringListRef StringListEditor::get_grt_string_list() {
  grt::StringListRef list(grt::Initialized);

  for (int c = _tree.count(), i = 0; i < c; i++) {
    list.insert(_tree.root_node()->get_child(i)->get_string(0));
  }

  return list;
}
