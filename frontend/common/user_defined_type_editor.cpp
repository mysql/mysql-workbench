/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/util_functions.h"
#include "base/string_utilities.h"
#include "base/ui_form.h"
#include "base/log.h"

#include "mforms/utilities.h"
#include "mforms/checkbox.h"

#include "grt/editor_base.h"
#include "grtdb/db_object_helpers.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"

#include "user_defined_type_editor.h"
#include "grtui/string_list_editor.h"
#include "model/wb_context_model.h"

DEFAULT_LOG_DOMAIN("udt");

using namespace base;

UserDefinedTypeEditor::UserDefinedTypeEditor(const workbench_physical_ModelRef &model)
  : mforms::Form(0, mforms::FormResizable),
    _model(model),
    _vbox(false),
    _type_list(mforms::TreeAllowReorderRows | mforms::TreeFlatList),
    _args_box(true),
    _flags_box(false),
    _button_box(true) {
  set_title(_("User Defined Types"));
  set_name("User Type Editor");
  setInternalName("user_type_editor");

  set_content(&_vbox);
  _vbox.set_spacing(8);

  _vbox.add(&_type_list, true, true);
  _vbox.add(&_table, false, true);
  _vbox.add(&_button_box, false, true);

  _table.set_row_count(4);
  _table.set_column_count(2);
  _table.set_row_spacing(8);
  _table.set_column_spacing(8);
  _table.set_padding(12);

  _type_list.add_column(mforms::StringColumnType, _("Type"), 150, false);
  _type_list.add_column(mforms::StringColumnType, _("Definition"), 200, false);
  _type_list.add_column(mforms::StringColumnType, _("Flags"), -1, false);
  _type_list.end_columns();
  scoped_connect(_type_list.signal_changed(), std::bind(&UserDefinedTypeEditor::selected_row, this));

  _namel.set_text(_("Name:"));
  _namel.set_text_align(mforms::MiddleRight);
  _table.add(&_namel, 0, 1, 0, 1, mforms::HFillFlag);
  _table.add(&_name, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
  _typel.set_text(_("Type:"));
  _typel.set_text_align(mforms::MiddleRight);
  _table.add(&_typel, 0, 1, 1, 2, mforms::HFillFlag);
  _table.add(&_type, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);
  _argsl.set_text(_("Arguments:"));
  _argsl.set_text_align(mforms::MiddleRight);
  _table.add(&_argsl, 0, 1, 2, 3, mforms::HFillFlag);
  _table.add(&_args_box, 1, 2, 2, 3, mforms::HFillFlag | mforms::VFillFlag | mforms::HExpandFlag);
  _args_box.set_spacing(4);
  _args_box.add(&_args, true, true);
  _args_box.add(&_args_edit, false, true);
  _args_edit.set_text("...");
  _args_edit.set_tooltip(_("Edit value list as string list."));
  scoped_connect(_args_edit.signal_clicked(), std::bind(&UserDefinedTypeEditor::edit_arguments, this));
  _flagsl.set_text(_("Flags:"));
  _flagsl.set_text_align(mforms::TopRight);
  _table.add(&_flagsl, 0, 1, 3, 4, mforms::HFillFlag | mforms::VFillFlag);
  _table.add(&_flags_box, 1, 2, 3, 4, mforms::HFillFlag | mforms::VFillFlag | mforms::HExpandFlag);
  _add_button.enable_internal_padding(true);
  _delete_button.enable_internal_padding(true);
  _ok_button.enable_internal_padding(true);
  _cancel_button.enable_internal_padding(true);

  _button_box.set_padding(12);
  _button_box.set_spacing(8);

  _add_button.set_text(_("Add"));
  _add_button.set_size(80, -1);
  _button_box.add(&_add_button, false, false);
  _delete_button.set_text(_("Delete"));
  _delete_button.set_size(80, -1);
  _button_box.add(&_delete_button, false, false);

  _ok_button.set_text(_("OK"));
  _ok_button.set_size(80, -1);
  _button_box.add_end(&_ok_button, false, false);
  _cancel_button.set_text(_("Cancel"));
  _cancel_button.set_size(80, -1);
  _button_box.add_end(&_cancel_button, false, false);

  set_size(480, 480);
  center();

  refresh();
  if (_type_list.count() > 0)
    _type_list.select_node(_type_list.root_node()->get_child(0));

  scoped_connect(_add_button.signal_clicked(), std::bind(&UserDefinedTypeEditor::add_clicked, this));
  scoped_connect(_delete_button.signal_clicked(), std::bind(&UserDefinedTypeEditor::delete_clicked, this));

  scoped_connect(_ok_button.signal_clicked(), std::bind(&UserDefinedTypeEditor::ok_clicked, this));
  scoped_connect(_cancel_button.signal_clicked(), std::bind(&UserDefinedTypeEditor::cancel_clicked, this));

  scoped_connect(_name.signal_changed(), std::bind(&UserDefinedTypeEditor::name_changed, this));
  scoped_connect(_args.signal_changed(), std::bind(&UserDefinedTypeEditor::args_changed, this));

  // fill simple type combo
  grt::ListRef<db_SimpleDatatype> types(_model->catalog()->simpleDatatypes());

  GrtVersionRef version = wb::WBContextUI::get()->get_wb()->get_model_context()->get_target_version();

  _type.clear();
  _valid_types.clear();
  for (grt::ListRef<db_SimpleDatatype>::const_iterator t = types.begin(); t != types.end(); ++t) {
    // filter out types not valid for the target version
    if (version.is_valid() && !bec::CatalogHelper::is_type_valid_for_version(*t, version))
      continue;

    _type.add_item((*t)->name());
    _valid_types.push_back(*t);
  }
  scoped_connect(_type.signal_changed(), std::bind(&UserDefinedTypeEditor::type_changed, this));

  selected_row();
}

void UserDefinedTypeEditor::refresh() {
  grt::ListRef<db_UserDatatype> udts(_model->catalog()->userDatatypes());

  _user_types.clear();
  _type_list.clear();
  for (grt::ListRef<db_UserDatatype>::const_iterator t = udts.begin(); t != udts.end(); ++t) {
    // skip system entries
    // ml: this concept is wrong. There are no "system user datatypes". User datatypes are those
    //     created by the user. All other types must be listed/handled as usual.
    //     Search also wb_component_physical.cpp for "create_builtin_user_datatypes" for more info.
    // if (!g_str_has_prefix((*t).id().c_str(), "com.mysql.rdbms.mysql.userdatatype"))
    {
      mforms::TreeNodeRef node = _type_list.add_node();
      node->set_string(0, (*t)->name().c_str());
      node->set_string(1, (*t)->sqlDefinition().c_str());
      node->set_string(2, (*t)->flags().c_str());

      _user_types.push_back(*t);
    }
  }
}

void UserDefinedTypeEditor::selected_row() {
  mforms::TreeNodeRef node = _type_list.get_selected_node();
  if (!node) {
    for (std::vector<mforms::CheckBox *>::iterator iter = _flags.begin(); iter != _flags.end(); ++iter) {
      _flags_box.remove(*iter);
      delete *iter;
    }
    _flags.clear();

    _name.set_value("");
    _name.set_enabled(false);

    _args.set_value("");
    _args.set_enabled(false);
    _args_edit.set_enabled(false);

    _type.set_enabled(false);
  } else {
    _name.set_value(node->get_string(0));
    _name.set_enabled(true);
    _args.set_enabled(true);
    _type.set_enabled(true);

    std::string typespec = node->get_string(1);
    std::vector<std::string> flags = base::split(node->get_string(2), " ");

    std::string::size_type arg_start = typespec.find('(');
    std::string type_name;
    if (arg_start == std::string::npos) {
      type_name = typespec;
      _args.set_value("");
    } else {
      type_name = typespec.substr(0, arg_start);
      std::string args;
      arg_start++;
      if (typespec[typespec.size() - 1] == ')')
        args = typespec.substr(arg_start, typespec.size() - 1 - arg_start);
      else
        args = typespec.substr(arg_start);

      _args.set_value(args);
    }

    db_SimpleDatatypeRef simpleType;
    int index = 0;
    for (std::vector<db_SimpleDatatypeRef>::const_iterator t = _valid_types.begin(); t != _valid_types.end(); ++t) {
      if (strcasecmp((*t)->name().c_str(), type_name.c_str()) == 0) {
        simpleType = *t;
        break;
      }
      ++index;
    }

    if (!simpleType.is_valid()) {
      index = 0;
      simpleType = _valid_types.front();
    }
    _type.set_selected(index);

    type_changed();

    int i = 0;
    for (grt::StringListRef::const_iterator iter = simpleType->flags().begin(); iter != simpleType->flags().end();
         ++iter, ++i) {
      for (std::vector<std::string>::const_iterator fl = flags.begin(); fl != flags.end(); ++fl) {
        if (g_ascii_strcasecmp(fl->c_str(), (*iter).c_str()) == 0) {
          _flags[i]->set_active(true);
          break;
        }
      }
    }
  }
}

void UserDefinedTypeEditor::flag_toggled() {
  int i = 0;
  std::string flags;
  for (std::vector<mforms::CheckBox *>::iterator iter = _flags.begin(); iter != _flags.end(); ++iter, ++i) {
    if ((*iter)->get_active()) {
      if (!flags.empty())
        flags.append(" ");
      flags.append((*iter)->getInternalName());
    }
  }

  mforms::TreeNodeRef node = _type_list.get_selected_node();
  if (node)
    node->set_string(2, flags);
}

void UserDefinedTypeEditor::type_changed() {
  if (_type.get_selected_index() >= 0) {
    db_SimpleDatatypeRef simpleType(_valid_types[_type.get_selected_index()]);
    // check the type definition if it has args
    switch (*simpleType->parameterFormatType()) {
      case 0:
        _args.set_enabled(false);
        _args_edit.set_enabled(false);
        _args.set_value("");
        break;
      case 10:
        _args.set_enabled(true);
        _args_edit.set_enabled(true);
        break;
      default:
        _args.set_enabled(true);
        _args_edit.set_enabled(false);
        break;
    }

    // update flags list
    for (std::vector<mforms::CheckBox *>::iterator iter = _flags.begin(); iter != _flags.end(); ++iter) {
      _flags_box.remove(*iter);
      delete *iter;
    }
    _flags.clear();

    for (grt::StringListRef::const_iterator iter = simpleType->flags().begin(); iter != simpleType->flags().end();
         ++iter) {
      mforms::CheckBox *ch = new mforms::CheckBox();
      ch->set_text(**iter);
      ch->set_name(**iter);
      scoped_connect(ch->signal_clicked(), std::bind(&UserDefinedTypeEditor::flag_toggled, this));
      _flags_box.add(ch, false, true);
      _flags.push_back(ch);
    }

    args_changed();
  }
}

void UserDefinedTypeEditor::add_clicked() {
  mforms::TreeNodeRef node = _type_list.add_node();
  node->set_string(0, "usertype");
  node->set_string(1, "INT(11)");
  _type_list.select_node(node);
  selected_row();

  _user_types.push_back(db_UserDatatypeRef());
}

//--------------------------------------------------------------------------------------------------

/**
 * Checks all currently defined tables in the model to see if the given column is used in any of them.
 * Returns true if so, otherwise false.
 */
bool UserDefinedTypeEditor::is_type_used(const db_UserDatatypeRef &type) {
  grt::ListRef<db_Schema> schemata(_model->catalog()->schemata());
  for (grt::ListRef<db_Schema>::const_iterator sche = schemata.begin(); sche != schemata.end(); ++sche) {
    grt::ListRef<db_Table> tables((*sche)->tables());
    for (grt::ListRef<db_Table>::const_iterator table = tables.begin(); table != tables.end(); ++table) {
      grt::ListRef<db_Column> columns((*table)->columns());
      for (grt::ListRef<db_Column>::const_iterator col = columns.begin(); col != columns.end(); ++col) {
        if ((*col)->userType() == type)
          return true;
      }
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

void UserDefinedTypeEditor::delete_clicked() {
  mforms::TreeNodeRef node = _type_list.get_selected_node();
  if (node) {
    int row = _type_list.get_selected_row();
    if (_user_types[row].is_valid() && is_type_used(_user_types[row])) {
      mforms::Utilities::show_error(
        _("Delete User Type"),
        strfmt(_("Type '%s' is used in a column and cannot be deleted."), node->get_string(0).c_str()), _("OK"));

      return;
    }
    node->remove_from_parent();

    _user_types.erase(_user_types.begin() + row);
  }
  selected_row();
}

//--------------------------------------------------------------------------------------------------

static bool is_missing(const grt::ObjectRef &item, const std::vector<db_UserDatatypeRef> &types) {
  for (std::vector<db_UserDatatypeRef>::const_iterator iter = types.begin(); iter != types.end(); ++iter) {
    if ((*iter).is_valid() && (*iter).id() == item.id())
      return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void UserDefinedTypeEditor::ok_clicked() {
  // Check if everything is ok and then commit.

  std::set<std::string> names;
  grt::ListRef<db_UserDatatype> udts(_model->catalog()->userDatatypes());

  // Check for duplicate or invalid names.
  for (int c = _type_list.count(), i = 0; i < c; i++) {
    std::string name = _type_list.root_node()->get_child(i)->get_string(0);
    bool ok = true;

    for (std::string::size_type i = 0; i < name.size(); i++) {
      if (!isalnum(name[i]) && name[i] != '_' && name[i] != ' ') {
        ok = false;
        break;
      }
    }

    if (name.empty() || !ok) {
      mforms::Utilities::show_error(
        _("Invalid Type Name"),
        strfmt(_("'%s' is not a valid type name. Names may only contain alpha-numeric characters, spaces and _"),
               name.c_str()),
        _("OK"));
      return;
    }

    if (names.find(name) != names.end()) {
      mforms::Utilities::show_error(_("Duplicate Type Name"),
                                    strfmt(_("There are two or more types with the same name '%s'."), name.c_str()),
                                    _("OK"));
      return;
    }
    names.insert(name);
  }

  // check for deleted types
  grt::AutoUndo undo;

  grt::remove_list_items_matching(udts, std::bind(is_missing, std::placeholders::_1, _user_types));

  // add new types, update existing ones
  for (size_t i = 0; i < _user_types.size(); i++) {
    db_UserDatatypeRef type(grt::Initialized);
    mforms::TreeNodeRef node(_type_list.node_at_row((int)i));
    if (!node)
      continue;

    if (!_user_types[i].is_valid())
      type = db_UserDatatypeRef(grt::Initialized);
    else
      type = _user_types[i];

    if (*type->name() != node->get_string(0))
      type->name(node->get_string(0));

    if (*type->sqlDefinition() != node->get_string(1)) {
      std::string typespec = node->get_string(1);
      type->sqlDefinition(typespec);
      std::string::size_type arg_start = typespec.find('(');
      std::string type_name;
      if (arg_start == std::string::npos)
        type_name = typespec;
      else
        type_name = typespec.substr(0, arg_start);

      db_SimpleDatatypeRef simpleType;
      for (std::vector<db_SimpleDatatypeRef>::const_iterator t = _valid_types.begin(); t != _valid_types.end(); ++t) {
        if (strcasecmp((*t)->name().c_str(), type_name.c_str()) == 0) {
          simpleType = *t;
          break;
        }
      }
      if (!simpleType.is_valid())
        logWarning("Could not find type %s for udt '%s'\n", type_name.c_str(), type->name().c_str());
      if (type->actualType() != simpleType)
        type->actualType(simpleType);
    }
    if (*type->flags() != node->get_string(2))
      type->flags(node->get_string(2));

    // this is a new type
    if (!_user_types[i].is_valid())
      udts.insert(type);
  }

  undo.end_or_cancel_if_empty(_("Edit User Defined Types"));

  if (wb::WBContextUI::get()->get_wb()->get_model_context())
    wb::WBContextUI::get()->get_wb()->get_model_context()->_udt_list_changed();

  close();
}

void UserDefinedTypeEditor::cancel_clicked() {
  close();
}

void UserDefinedTypeEditor::name_changed() {
  mforms::TreeNodeRef node = _type_list.get_selected_node();
  if (node)
    node->set_string(0, _name.get_string_value());
}

void UserDefinedTypeEditor::args_changed() {
  std::string typespec = _type.get_string_value();
  std::string args = _args.get_string_value();
  mforms::TreeNodeRef node = _type_list.get_selected_node();

  if (node) {
    if (args.empty())
      node->set_string(1, typespec);
    else
      node->set_string(1, typespec + "(" + args + ")");
  }
}

void UserDefinedTypeEditor::edit_arguments() {
  grtui::StringListEditor editor(this, true);
  editor.set_title(_("Edit Type Arguments"));

  std::vector<std::string> list;

  gchar **parts = g_strsplit(_args.get_string_value().c_str(), ",", -1);
  if (parts) {
    for (gchar **p = parts; *p; ++p) {
      *p = g_strstrip(*p);
      if (**p == '\'') {
        std::memmove(*p, *p + 1, strlen(*p)); // copy the ending 0 too
        if (g_str_has_suffix(*p, "'"))
          *strrchr(*p, '\'') = 0;
      }
      list.push_back(*p);
    }

    g_strfreev(parts);
  }

  editor.set_string_list(list);

  if (editor.run()) {
    list = editor.get_string_list();

    std::string args;
    for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
      if (!args.empty())
        args.append(", ");

      args.append("'").append(*i).append("'");
    }

    _args.set_value(args);

    args_changed();
  }
}
