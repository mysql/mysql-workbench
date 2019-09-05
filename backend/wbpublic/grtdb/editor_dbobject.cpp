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

#include "editor_dbobject.h"

#include "base/util_functions.h"

#include "grt/validation_manager.h"
#include "grtpp_notifications.h"

#include "base/string_utilities.h"
#include "base/notifications.h"

#include "mforms/utilities.h"
#include "mforms/code_editor.h"

#include "sqlide/sql_editor_be.h"
#include "db_helpers.h"

#include "mysql/MySQLRecognizerCommon.h"

const char *DEFAULT_CHARSET_CAPTION = "Default Charset";
const char *DEFAULT_COLLATION_CAPTION = "Default Collation";

using namespace bec;
using namespace parsers;

//--------------------------------------------------------------------------------------------------

DBObjectEditorBE::DBObjectEditorBE(const db_DatabaseObjectRef &object)
: BaseEditor(object) {
  _ignored_object_fields_for_ui_refresh.insert("lastChangeDate");

  // Get the owning catalog.
  GrtObjectRef run = object;
  while (run.is_valid() && !run.is_instance(db_Catalog::static_class_name()))
    run = run->owner();

  _catalog = db_CatalogRef::cast_from(run);

  _parserServices = MySQLParserServices::get();
  bool case_sensitive = true;
  if (object->customData().get_int("CaseSensitive", 1) == 0)
    case_sensitive = false;

  // Assume a default version if the given catalog is incomplete.
  GrtVersionRef version = get_catalog()->version();
  if (!version.is_valid())
    version = bec::parse_version("8.0.16");
  _globalSymbols = parsers::functionSymbolsForVersion(bec::versionToEnum(version));

  std::string sqlMode;
  if (object->customData().has_key("sqlMode"))
    sqlMode = object->customData().get_string("sqlMode");
  _parserContext =
    _parserServices->createParserContext(get_catalog()->characterSets(), version, sqlMode, case_sensitive);

  // Because syntax checks and auto completion are done in different threads we need 2 different parser contexts.
  _autocompletionContext =
    _parserServices->createParserContext(get_catalog()->characterSets(), version, sqlMode, case_sensitive);

  _val_notify_conn = ValidationManager::signal_notify()->connect(
    std::bind(&DBObjectEditorBE::notify_from_validation, this, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3, std::placeholders::_4));

  // Get notified about version number changes.
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNPreferencesDidClose");

  grt::DictRef info(true);
  info.gset("form", form_id());
  info.set("object", object);

  // Must be delayed, because observer will probably need the form to be finished constructing
  grt::GRTNotificationCenter::get()->send_grt("GRNDBObjectEditorCreated", grt::ObjectRef(), info);
}

//--------------------------------------------------------------------------------------------------

DBObjectEditorBE::~DBObjectEditorBE() {
  grt::GRTNotificationCenter::get()->remove_grt_observer(this);
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info) {
  if (info.get_int("saved") == 1) {
    if (name == "GRNPreferencesDidClose") {
      // We want to see changes for the server version.
      GrtVersionRef version = get_catalog()->version();
      _parserContext->updateServerVersion(version);
      get_sql_editor()->setServerVersion(version);
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool DBObjectEditorBE::is_editing_live_object() {
  return get_dbobject()->customData().get("liveRdbms").is_valid();
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::apply_changes_to_live_object() {
  BaseEditor::apply_changes_to_live_object();

  if (on_apply_changes_to_live_object(this, false))
    refresh_live_object();
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::refresh_live_object() {
  BaseEditor::refresh_live_object();

  on_refresh_live_object(this);
}

//--------------------------------------------------------------------------------------------------

bool DBObjectEditorBE::can_close() {
  // Editing in a model always allows to close the editor. Save checks are done when the model
  // is closed.
  if (!is_editing_live_object())
    return true;

  bool res = BaseEditor::can_close();

  // Note: the result of the BaseEditor::can_close() is only used if there's no apply callback set.
  //       Otherwise we always use the callback for checks (because there can be other changes than
  //       just the code editor which is checked in the BaseEditor).
  if (on_apply_changes_to_live_object) {
    bool is_object_modified = on_apply_changes_to_live_object(this, true);
    if (is_object_modified) {
      int user_choice =
        mforms::Utilities::show_warning(base::strfmt(_("Object %s was changed"), get_name().c_str()),
                                        base::strfmt(_("Do you want to save changes made to %s?"), get_name().c_str()),
                                        _("Save"), _("Cancel"), _("Don't Save"));

      if (mforms::ResultOk == user_choice)
        res = on_apply_changes_to_live_object(this, false);
      else if (mforms::ResultCancel == user_choice)
        res = false;
      else
        res = true;
    } else
      res = true;
  }
  return res;
}

//--------------------------------------------------------------------------------------------------

bool DBObjectEditorBE::should_close_on_delete_of(const std::string &oid) {
  if (get_object().id() == oid)
    return true;

  db_SchemaRef schema(get_schema());
  if (schema.is_valid() && schema.id() == oid)
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::update_change_date() {
  get_object().set_member("lastChangeDate", grt::StringRef(base::fmttime(0, DATETIME_FMT)));
}

//--------------------------------------------------------------------------------------------------

std::string DBObjectEditorBE::get_name() {
  return get_object()->name();
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::set_name(const std::string &name) {
  if (get_object()->name() != name) {
    RefreshUI::Blocker refresh_block(*this);

    AutoUndoEdit undo(this, get_dbobject(), "name");

    std::string name_ = base::trim(name);
    get_dbobject()->name(name_);

    update_change_date();
    undo.end(base::strfmt(_("Rename to '%s'"), name_.c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

std::string DBObjectEditorBE::get_comment() {
  return get_dbobject()->comment();
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::set_comment(const std::string &descr) {
  if (get_dbobject()->comment() != descr) {
    RefreshUI::Blocker blocker(*this);
    AutoUndoEdit undo(this, get_dbobject(), "comment");

    get_dbobject()->comment(descr);

    update_change_date();
    undo.end(_("Edit Comment"));
  }
}

//--------------------------------------------------------------------------------------------------

std::string DBObjectEditorBE::get_sql() {
  if (db_DatabaseDdlObjectRef::can_wrap(get_object())) {
    db_DatabaseDdlObjectRef object = db_DatabaseDdlObjectRef::cast_from(get_object());
    return object->sqlDefinition();
  }

  return "";
}

//--------------------------------------------------------------------------------------------------

/**
 * Called from outside to set new sql text in our editor.
 */
void DBObjectEditorBE::set_sql(const std::string &sql) {
  get_sql_editor()->sql(sql.c_str());
  commit_changes();
  send_refresh();
}

//--------------------------------------------------------------------------------------------------

bool DBObjectEditorBE::is_sql_commented() {
  return (*get_dbobject()->commentedOut() != 0);
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::set_sql_commented(bool flag) {
  RefreshUI::Blocker blocker(*this);

  AutoUndoEdit undo(this, get_dbobject(), "commentedOut");

  get_dbobject()->commentedOut(flag ? 1 : 0);

  update_change_date();
  undo.end(_("Comment Out SQL"));
}

//--------------------------------------------------------------------------------------------------

db_CatalogRef DBObjectEditorBE::get_catalog() {
  return _catalog;
}

//--------------------------------------------------------------------------------------------------

db_SchemaRef DBObjectEditorBE::get_schema() {
  GrtObjectRef object = get_dbobject();

  while (object.is_valid() && !object.is_instance(db_Schema::static_class_name()))
    object = object->owner();

  return db_SchemaRef::cast_from(object);
}

//--------------------------------------------------------------------------------------------------

std::string DBObjectEditorBE::get_schema_name() {
  return get_schema()->name();
}

//--------------------------------------------------------------------------------------------------

db_SchemaRef DBObjectEditorBE::get_schema_with_name(const std::string &schema_name) {
  return grt::find_named_object_in_list(_catalog->schemata(), schema_name);
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> DBObjectEditorBE::get_all_schema_names() {
  std::vector<std::string> names;
  if (is_editing_live_object()) {
    names.push_back(get_schema()->name());
    return names;
  }
  grt::ListRef<db_Schema> schema_list = _catalog->schemata();

  for (size_t sc = schema_list.count(), s = 0; s < sc; s++)
    names.push_back(schema_list[s]->name());

  return names;
}

//--------------------------------------------------------------------------------------------------

/**
 * Collects the FQN for all tables not in our own schema.
 */
std::vector<std::string> DBObjectEditorBE::get_all_table_names() {
  if (is_editing_live_object())
    on_create_live_table_stubs(this);

  grt::ListRef<db_Schema> schema_list = _catalog->schemata();
  db_SchemaRef myschema = get_schema();
  std::vector<std::string> table_list;

  // Construct FQN for all tables. This will sort all tables within the same schema together.
  table_list = get_schema_table_names();

  for (size_t i = 0; i < schema_list.count(); ++i) {
    if (schema_list[i] == myschema)
      continue;

    db_SchemaRef schema = schema_list[i];
    std::string schema_name = schema_list[i]->name();

    for (size_t j = 0; j < schema->tables().count(); ++j)
      table_list.push_back("`" + schema_name + "`.`" + *schema->tables()[j]->name() + "`");
  }

  std::sort(table_list.begin(), table_list.end());
  table_list.push_back("Specify Manually...");

  return table_list;
}

//--------------------------------------------------------------------------------------------------

/**
 * Collects the FQN for all tables in our schema.
 */
std::vector<std::string> DBObjectEditorBE::get_schema_table_names() {
  db_SchemaRef schema = get_schema();
  std::vector<std::string> table_list;
  std::string schema_name = schema->name();

  if (schema.is_valid()) {
    for (size_t i = 0; i < schema->tables().count(); ++i)
      table_list.push_back("`" + schema_name + "`.`" + *schema->tables()[i]->name() + "`");
  }

  std::sort(table_list.begin(), table_list.end());

  return table_list;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> DBObjectEditorBE::get_table_column_names(const std::string &fq_table_name) {
  db_SchemaRef schema;
  std::vector<std::string> columns;

  if (fq_table_name.empty())
    return columns;

  std::vector<std::string> parts = base::split_qualified_identifier(fq_table_name);
  std::string table_name;

  if (parts.size() == 1) {
    table_name = parts[0];
    schema = get_schema();
  } else if (!parts.empty()) {
    schema = get_schema_with_name(parts[0]);
    table_name = parts[1];
  }

  if (schema.is_valid()) {
    db_TableRef table(grt::find_named_object_in_list(schema->tables(), table_name));

    if (table.is_valid()) {
      for (size_t c = table->columns().count(), i = 0; i < c; i++)
        columns.push_back(*table->columns()[i]->name());
    }
  }

  return columns;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> DBObjectEditorBE::get_table_column_names(const db_TableRef &table) {
  std::vector<std::string> columns;

  if (table.is_valid())
    for (size_t c = table->columns().count(), i = 0; i < c; i++)
      columns.push_back(*table->columns()[i]->name());

  return columns;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> DBObjectEditorBE::get_charset_list() {
  std::vector<std::string> result;
  grt::ListRef<db_CharacterSet> charsets = _catalog->characterSets();

  for (size_t j = 0; j < charsets.count(); ++j) {
    db_CharacterSetRef cs = charsets.get(j);
    std::string cs_name(cs->name().c_str());

    result.push_back(cs_name);
  }

  result.push_back(DEFAULT_CHARSET_CAPTION);
  std::sort(result.begin(), result.end());

  return result;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> DBObjectEditorBE::get_charset_collation_list(const std::string &charset) {
  std::vector<std::string> result;
  grt::ListRef<db_CharacterSet> charsets = _catalog->characterSets();

  for (size_t j = 0; j < charsets.count(); ++j) {
    db_CharacterSetRef cs = charsets.get(j);
    if (cs->name() != charset)
      continue;

    grt::StringListRef collations(cs->collations());
    for (size_t k = 0; k < collations.count(); ++k) {
      result.push_back(collations.get(k));
    }
  }

  result.push_back(DEFAULT_COLLATION_CAPTION);
  std::sort(result.begin(), result.end());

  return result;
}
//--------------------------------------------------------------------------------------------------

std::vector<std::string> DBObjectEditorBE::get_charset_collation_list() {
  std::vector<std::string> collation_list;
  grt::ListRef<db_CharacterSet> charsets = _catalog->characterSets();

  for (size_t j = 0; j < charsets.count(); ++j) {
    db_CharacterSetRef cs = charsets.get(j);
    grt::StringListRef collations(cs->collations());
    std::string cs_name(cs->name().c_str());

    collation_list.push_back(format_charset_collation(cs_name, ""));

    for (size_t k = 0; k < collations.count(); ++k)
      collation_list.push_back(format_charset_collation(cs_name, collations.get(k)));
  }

  return collation_list;
}

//--------------------------------------------------------------------------------------------------

std::string DBObjectEditorBE::format_charset_collation(const std::string &charset, const std::string &collation) {
  if (collation.empty()) {
    if (charset.empty())
      return " - ";
    else
      return charset + " - " + DEFAULT_COLLATION_CAPTION;
  } else
    return charset + " - " + collation;
}

//--------------------------------------------------------------------------------------------------

bool DBObjectEditorBE::parse_charset_collation(const std::string &str, std::string &charset, std::string &collation) {
  std::string::size_type pos;
  if ((pos = str.find(" - ")) != std::string::npos) {
    charset = str.substr(0, pos);
    collation = str.substr(pos + 3);
    if (collation == DEFAULT_COLLATION_CAPTION)
      collation = "";

    return true;
  }

  charset = "";
  collation = "";

  return false;
}

//--------------------------------------------------------------------------------------------------

bool DBObjectEditorBE::has_editor() {
  if (_sql_editor)
    return true;
  return false;
}

//--------------------------------------------------------------------------------------------------

MySQLEditor::Ref DBObjectEditorBE::get_sql_editor() {
  if (!_sql_editor) {
    _sql_editor = MySQLEditor::create(_parserContext, _autocompletionContext, { _globalSymbols });
    grt::DictRef obj_options = get_dbobject()->customData();
    if (obj_options.has_key("sqlMode"))
      _sql_editor->set_sql_mode(obj_options.get_string("sqlMode"));
  }
  return _sql_editor;
}

//--------------------------------------------------------------------------------------------------

void bec::DBObjectEditorBE::reset_editor_undo_stack() {
  // Don't create an editor control if we don't need one (e.g. for the schema editor).
  if (_sql_editor)
    _sql_editor->get_editor_control()->reset_dirty();
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::notify_from_validation(const grt::Validator::Tag &tag, const grt::ObjectRef &obj,
                                              const std::string &msg, const int level) {
  bool notify_is_for_us = false;

  if (obj.is_valid()) {
    // Get edited object
    const GrtObjectRef our_obj = get_object();
    const GrtObjectRef val_obj = GrtObjectRef::cast_from(obj);

    // Check if passed object is ours
    if (our_obj == val_obj)
      notify_is_for_us = true;
    else {
      GrtObjectRef parent = val_obj->owner();
      while (parent.is_valid()) {
        if (parent == our_obj) {
          notify_is_for_us = true;
          break;
        }
        parent = parent->owner();
      }
      // Scan owners upwards
    }
  } else {
    if (tag == "*")
      notify_is_for_us = true;
  }

  if (notify_is_for_us) {
    _last_validation_check_status = (grt::MessageType)level;
    _last_validation_message = msg;
  }
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::send_refresh() {
  db_DatabaseObjectRef db_object = get_dbobject();
  (*db_object->signal_changed())("", grt::ValueRef());
}

//--------------------------------------------------------------------------------------------------

void DBObjectEditorBE::set_sql_mode(const std::string &value) {
  MySQLEditor::Ref sql_editor = get_sql_editor();
  if (sql_editor)
    sql_editor->set_sql_mode(value);
}

//--------------------------------------------------------------------------------------------------
