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

#include <grts/structs.db.h>

#include "base/string_utilities.h"
#include <grtpp_undo_manager.h>

#include "grt/parse_utils.h"
#include "grt/common.h"
#include "grtdb/db_object_helpers.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace base;

//================================================================================
// db_Column

static void notify_visible_member_change(const std::string &member, const grt::ValueRef &ovalue, db_Column *ref) {
  if (member == "name" || member == "simpleType" || member == "userType") {
    if (ovalue != ref->get_member(member) && ref->owner().is_valid())
      (*db_TableRef::cast_from(ref->owner())->signal_refreshDisplay())("column");
  }
}

void db_Column::init() {
  // No need to disconnect management since the signal is part of the object.
  _changed_signal.connect(std::bind(notify_visible_member_change, std::placeholders::_1, std::placeholders::_2, this));
}

db_Column::~db_Column() {
}

template <class T>
class auto_array_ptr {
  T *_ptr;

public:
  auto_array_ptr(T *ptr) : _ptr(ptr) {
  }
  ~auto_array_ptr() {
    delete[] _ptr;
  }

  operator T *() {
    return _ptr;
  }
};

grt::StringRef db_Column::formattedRawType() const {
  if (userType().is_valid()) {
    std::string arguments;

    // if no simple or structured datatype is set,
    // simply take the parameters
    if (length() != bec::EMPTY_COLUMN_LENGTH) {
      arguments = strfmt("(%i)", (int)length());
    } else if (precision() != bec::EMPTY_COLUMN_PRECISION) {
      std::string tmp;
      if (scale() != bec::EMPTY_COLUMN_SCALE)
        tmp = strfmt("(%i,%i)", (int)precision(), (int)scale());
      else
        tmp = strfmt("(%i)", (int)precision());
      arguments = tmp;
    } else if (datatypeExplicitParams().is_valid() && *datatypeExplicitParams() != "") {
      arguments = *datatypeExplicitParams();
    }

    return std::string(userType()->name()).append(arguments);
  } else
    return formattedType();
}

grt::StringRef db_Column::formattedType() const {
  db_SimpleDatatypeRef simpleType(this->simpleType());
  db_StructuredDatatypeRef structuredType(this->structuredType());
  std::string caption;

  if (simpleType.is_valid()) {
    ssize_t ptype = simpleType->parameterFormatType();
    caption = simpleType->name();

    if (simpleType->numericPrecision() != bec::EMPTY_TYPE_PRECISION) {
      std::string tmp;
      if (precision() != bec::EMPTY_COLUMN_PRECISION && scale() != bec::EMPTY_COLUMN_SCALE &&
          (ptype == 3 || ptype == 4 || ptype == 5 || ptype == 6))
        tmp = strfmt("(%i,%i)", (int)precision(), (int)scale());
      else if (precision() != bec::EMPTY_COLUMN_PRECISION && (ptype == 1 || ptype == 2 || ptype == 4 || ptype == 6))
        tmp = strfmt("(%i)", (int)precision());
      caption.append(tmp);
    } else {
      if (*simpleType->characterMaximumLength() != bec::EMPTY_TYPE_MAXIMUM_LENGTH ||
          *simpleType->characterOctetLength() != bec::EMPTY_TYPE_OCTET_LENGTH) {
        if (length() != bec::EMPTY_COLUMN_LENGTH && (ptype == 1 || ptype == 2 || ptype == 4 || ptype == 6)) {
          caption.append(strfmt("(%i)", (int)length()));
        }
      } else if (*simpleType->dateTimePrecision() > 0 && length() > 0) {
        // timestamp, time, datetime, year
        caption.append(strfmt("(%i)", (int)length()));
      } else if (datatypeExplicitParams().is_valid() && *datatypeExplicitParams() != "")
        caption.append(*datatypeExplicitParams());
    }
  } else if (structuredType.is_valid()) {
  } else {
    std::string arguments;

    if (userType().is_valid())
      caption = userType()->sqlDefinition();

    // if no simple or structured datatype is set,
    // simply take the parameters
    if (length() != bec::EMPTY_COLUMN_LENGTH) {
      arguments = strfmt("(%i)", (int)length());
    } else if (precision() != bec::EMPTY_COLUMN_PRECISION) {
      std::string tmp;
      if (scale() != bec::EMPTY_COLUMN_SCALE)
        tmp = strfmt("(%i,%i)", (int)precision(), (int)scale());
      else
        tmp = strfmt("(%i)", (int)precision());
      arguments = tmp;
    } else if (datatypeExplicitParams().is_valid() && *datatypeExplicitParams() != "") {
      arguments = *datatypeExplicitParams();
    }

    if (!arguments.empty()) {
      std::string::size_type p;
      if ((p = caption.find('(')) != std::string::npos)
        caption = caption.substr(0, p);

      caption.append(arguments);
    }
  }

  return caption;
}

void db_Column::formattedType(const grt::StringRef &value) {
  if (formattedType() == value.c_str())
    return;
}

/** Sets the datatype defined by a string to the column.
 *
 * Assigns a datatype defined by a string like NUMERIC(7, 2) to the given column.
 *
 * @return 1 on success or 0 on parse error or invalid type/invalid params
 */
grt::IntegerRef db_Column::setParseType(const std::string &type, const grt::ListRef<db_SimpleDatatype> &typeList) {
  grt::ListRef<db_UserDatatype> user_types;
  grt::ListRef<db_SimpleDatatype> default_type_list;
  GrtVersionRef targetVersion(grt::Initialized);
  if (owner().is_valid() && owner()->owner().is_valid() && owner()->owner()->owner().is_valid()) {
    db_CatalogRef catalog = db_CatalogRef::cast_from(owner()->owner()->owner());
    user_types = catalog->userDatatypes();
    default_type_list = catalog->simpleDatatypes();
    GrtVersionRef catalogVersion = GrtVersionRef::cast_from(bec::getModelOption(workbench_physical_ModelRef::cast_from(catalog->owner()), "CatalogVersion"));
    targetVersion->majorNumber(catalogVersion->majorNumber());
    targetVersion->minorNumber(catalogVersion->minorNumber());
    targetVersion->releaseNumber(catalogVersion->releaseNumber() > 0 ? catalogVersion->releaseNumber()
                                                                     : grt::IntegerRef(99));
    targetVersion->buildNumber(catalogVersion->buildNumber());
    targetVersion->status(catalogVersion->status());
  }

  db_UserDatatypeRef userType;
  db_SimpleDatatypeRef simpleType;
  int precision = bec::EMPTY_COLUMN_PRECISION;
  int scale = bec::EMPTY_COLUMN_SCALE;
  int length = bec::EMPTY_COLUMN_LENGTH;
  std::string datatypeExplicitParams;
  grt::AutoUndo undo(!is_global());

  // If the available release number is negative, that's meant to signify "any release number".
  parsers::MySQLParserServices *services = parsers::MySQLParserServices::get();
  if (!services->parseTypeDefinition(type, targetVersion, typeList, user_types, default_type_list,
      simpleType, userType, precision, scale, length, datatypeExplicitParams))
      return 0;
  this->userType(userType);
  this->simpleType(simpleType);
  this->precision(precision);
  this->scale(scale);
  this->length(length);
  this->datatypeExplicitParams(datatypeExplicitParams);

  if (_owner.is_valid())
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("column");

  undo.end(_("Change Column Type"));

  return 1;
}
