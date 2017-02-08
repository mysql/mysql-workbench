/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/tree_model.h"

#include "mysql_schema_editor.h"

#include "base/log.h"

#include "mforms/utilities.h"

DEFAULT_LOG_DOMAIN("SchemaEditor");

//--------------------------------------------------------------------------------------------------

MySQLSchemaEditorBE::MySQLSchemaEditorBE(const db_mysql_SchemaRef &schema) : bec::SchemaEditorBE(schema) {
  _initial_name = schema->name();
  _schema = schema;
}

//--------------------------------------------------------------------------------------------------

void MySQLSchemaEditorBE::refactor_catalog_upon_schema_rename(const std::string &old_name,
                                                              const std::string &new_name) {
  try {
    bec::AutoUndoEdit undo(this);

    db_mysql_CatalogRef catalog = db_mysql_CatalogRef::cast_from(get_catalog());
    _parser_services->renameSchemaReferences(_parser_context, catalog, old_name, new_name);

    undo.end(base::strfmt(_("Update references to schema: `%s` -> `%s`"), old_name.c_str(), new_name.c_str()));
  } catch (std::exception &exc) {
    logError("Exception refactoring for schema rename: %s\n", exc.what());
    mforms::Utilities::show_error(
      "Refactor Schema", base::strfmt("An error occurred while renaming objects from the schema.\n%s", exc.what()),
      "OK", "", "");
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLSchemaEditorBE::refactor_catalog() {
  try {
    bec::AutoUndoEdit undo(this);
    std::string from_name = get_schema()->customData().get_string("LastRefactoringTargetName", get_schema()->oldName());
    std::string to_name = get_schema()->name();
    if (from_name.empty())
      from_name = _initial_name;

    db_mysql_CatalogRef catalog = db_mysql_CatalogRef::cast_from(get_catalog());
    _parser_services->renameSchemaReferences(_parser_context, catalog, from_name, to_name);

    get_schema()->customData().set("LastRefactoringTargetName", grt::StringRef(to_name));

    undo.end(base::strfmt(_("Update references to schema: `%s` -> `%s`"), from_name.c_str(), to_name.c_str()));

    mforms::Utilities::show_message(
      "Refactor Schema",
      base::strfmt("Schema objects references changed from `%s` changed to `%s`.", from_name.c_str(), to_name.c_str()),
      "OK");
  } catch (std::exception &exc) {
    logError("Exception refactoring for schema rename: %s\n", exc.what());
    mforms::Utilities::show_error("Refactor Schema",
                                  base::strfmt("An error occurred while changing object references.\n%s", exc.what()),
                                  "OK", "", "");
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLSchemaEditorBE::refactor_possible() {
  std::string from_name = get_schema()->customData().get_string("LastRefactoringTargetName", get_schema()->oldName());
  std::string to_name = get_schema()->name();
  if (from_name.empty())
    from_name = _initial_name;

  return !is_editing_live_object() && from_name != to_name;
}

//--------------------------------------------------------------------------------------------------
