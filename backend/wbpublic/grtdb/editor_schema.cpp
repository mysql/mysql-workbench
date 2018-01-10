/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "editor_schema.h"
#include "base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

SchemaEditorBE::SchemaEditorBE(const db_SchemaRef& schema) : DBObjectEditorBE(schema) {
}

//--------------------------------------------------------------------------------------------------

void SchemaEditorBE::set_name(const std::string& name) {
  if (is_editing_live_object() && get_schema()->oldName() != "")
    return;
  DBObjectEditorBE::set_name(name);
}

//--------------------------------------------------------------------------------------------------

void SchemaEditorBE::set_schema_option_by_name(const std::string& name, const std::string& value) {
  if (name.compare("CHARACTER SET - COLLATE") == 0) {
    // Shortcut that sets both CHARACTER SET and COLLATE separated by a dash.
    if (value != get_schema_option_by_name(name)) {
      std::string charset, collation;
      parse_charset_collation(value, charset, collation);
      if (charset != *get_schema()->defaultCharacterSetName() || collation != *get_schema()->defaultCollationName()) {
        RefreshUI::Blocker block(*this);
        AutoUndoEdit undo(this);
        get_schema()->defaultCharacterSetName(charset);
        get_schema()->defaultCollationName(collation);
        update_change_date();
        undo.end(strfmt(_("Change Charset/Collation for '%s'"), get_schema()->name().c_str()));
      }
    }
  } else if (name.compare("CHARACTER SET") == 0) {
    AutoUndoEdit undo(this);

    get_schema()->defaultCharacterSetName(value);
    update_change_date();

    undo.end(strfmt(_("Set Default Character Set for Schema '%s'"), get_name().c_str()));
  } else if (name.compare("COLLATE") == 0) {
    AutoUndoEdit undo(this);

    get_schema()->defaultCollationName(grt::StringRef(value));
    update_change_date();

    undo.end(strfmt(_("Set Default Collation for Schema '%s'"), get_name().c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

std::string SchemaEditorBE::get_schema_option_by_name(const std::string& name) {
  if (name.compare("CHARACTER SET") == 0)
    return get_schema()->defaultCharacterSetName();
  else if (name.compare("COLLATE") == 0)
    return get_schema()->defaultCollationName();
  else if (name.compare("CHARACTER SET - COLLATE") == 0)
    return format_charset_collation(get_schema()->defaultCharacterSetName(), get_schema()->defaultCollationName());

  return std::string();
}

//--------------------------------------------------------------------------------------------------

std::string SchemaEditorBE::get_title() {
  return get_name() + " - Schema";
}

//--------------------------------------------------------------------------------------------------
