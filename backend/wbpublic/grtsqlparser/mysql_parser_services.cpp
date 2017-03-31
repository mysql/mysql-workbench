/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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
