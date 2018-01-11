/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "grtdb/db_object_helpers.h"

#include "mysql_parser_services.h"

using namespace parsers;

//------------------ MySQLParserServices -----------------------------------------------------------

MySQLParserServices::Ref MySQLParserServices::get() {
  MySQLParserServices::Ref module =
    dynamic_cast<MySQLParserServices::Ref>(grt::GRT::get()->get_module("MySQLParserServices"));
  if (module == nullptr)
    throw std::runtime_error("Can't get MySQLParserServices module.");
  return module;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Compares the given typename with what is in the type list, including the synonyms and returns
 *	the type whose name or synonym matches.
 */
db_SimpleDatatypeRef MySQLParserServices::findDataType(SimpleDatatypeListRef types, GrtVersionRef targetVersion,
                                                       const std::string &name) {
  for (auto type : types) {
    bool typeFound = base::same_string(type->name(), name, false);
    if (!typeFound) {
      // Type has not the default name, but maybe one of the synonyms.
      for (auto synonym : type->synonyms()) {
        if (base::same_string(*synonym, name, false)) {
          typeFound = true;
          break;
        }
      }
    }

    if (typeFound) {
      if (!targetVersion.is_valid() || bec::CatalogHelper::is_type_valid_for_version(type, targetVersion))
        return type;
    }
  }
  return db_SimpleDatatypeRef();
}

//--------------------------------------------------------------------------------------------------
