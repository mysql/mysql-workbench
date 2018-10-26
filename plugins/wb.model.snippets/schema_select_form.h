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

#ifndef _SCHEMA_SELECT_FORM_INCLUDED_
#define _SCHEMA_SELECT_FORM_INCLUDED_

#include "grtui/gui_plugin_base.h"

class SchemaSelectionForm : public GUIPluginBase, public mforms::Form {
public:
  SchemaSelectionForm(grt::Module *module, grt::ListRef<db_Schema> schemas, db_SchemaRef default_schema)
    : GUIPluginBase(module),
      Form(NULL, mforms::FormResizable),
      _box(false),
      _button_box(true),
      _schema_list(false),
      _schemas(schemas) {
    set_title(_("Select Destination Schema"));
    set_name("Schema Selection");
    _box.set_spacing(8);
    _box.set_padding(8);
    _button_box.add(&_ok_button, true, true);
    _button_box.add(&_cancel_button, true, true);
    _cancel_button.set_text(_("Cancel"));
    _ok_button.set_text(_("Select"));
    _schema_list.set_size(200, -1);
    _schema_list.set_heading("Schemas");
    for (size_t sz = _schemas.count(), i = 0; i < sz; i++) {
      _schema_list.add_item(_schemas[i]->name());
      if (default_schema->name() == _schemas[i]->name()) {
        _schema_list.set_selected((int)i);
      };
    };
    if (_schema_list.get_selected_index() < 0) {
      _schema_list.add_item("Add new schema");
      _schema_list.set_selected((int)_schemas.count());
    };
    _box.add(&_schema_list, true, true);
    _button_box.set_spacing(8);
    _button_box.set_padding(8);
    _button_box.set_homogeneous(true);

    _box.add_end(&_button_box, false, true);
    set_content(&_box);
  };

  db_SchemaRef get_selection() {
    if (_schema_list.get_selected_index() == (int)_schemas.count())
      return db_SchemaRef();
    return _schemas[_schema_list.get_selected_index()];
  };

  bool run() {
    center();
    int x;
    x = run_modal(&_ok_button, &_cancel_button);
    mforms::View::show(false);
    return x > 0;
  }

protected:
  mforms::Box _box;
  mforms::Box _button_box;
  mforms::Button _cancel_button;
  mforms::Button _ok_button;
  mforms::ListBox _schema_list;
  grt::ListRef<db_Schema> _schemas;
};

#endif //#ifndef _SCHEMA_SELECT_FORM_INCLUDED_
