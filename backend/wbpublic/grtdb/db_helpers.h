/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "wbpublic_public_interface.h"

#include "base/symbol-info.h"

#include "grts/structs.db.mgmt.h"
#include <string>

typedef grt::ListRef<db_CharacterSet> GrtCharacterSetsRef;
typedef grt::ListRef<db_SimpleDatatype> SimpleDatatypeListRef;
typedef grt::ListRef<db_UserDatatype> UserDatatypeListRef;

namespace bec {
  std::string WBPUBLICBACKEND_PUBLIC_FUNC get_host_identifier_for_connection(const db_mgmt_ConnectionRef &connection);
  std::string WBPUBLICBACKEND_PUBLIC_FUNC get_description_for_connection(const db_mgmt_ConnectionRef &conn);

  std::string WBPUBLICBACKEND_PUBLIC_FUNC sanitize_server_version_number(const std::string &version);

  GrtVersionRef WBPUBLICBACKEND_PUBLIC_FUNC parse_version(const std::string &version);
  int WBPUBLICBACKEND_PUBLIC_FUNC version_to_int(const GrtVersionRef &version);
  base::MySQLVersion WBPUBLICBACKEND_PUBLIC_FUNC versionToEnum(const GrtVersionRef &version);
  GrtVersionRef WBPUBLICBACKEND_PUBLIC_FUNC intToVersion(int version);
  bool WBPUBLICBACKEND_PUBLIC_FUNC version_equal(GrtVersionRef a, GrtVersionRef b);
  bool WBPUBLICBACKEND_PUBLIC_FUNC version_greater(GrtVersionRef a, GrtVersionRef b);

  bool WBPUBLICBACKEND_PUBLIC_FUNC is_supported_mysql_version_at_least(const std::string &mysql_version, int major,
                                                                       int minor, int release = -1);
  bool WBPUBLICBACKEND_PUBLIC_FUNC is_supported_mysql_version_at_least(int mysql_major, int mysql_minor,
                                                                       int mysql_release, int major, int minor,
                                                                       int release = -1);
  bool WBPUBLICBACKEND_PUBLIC_FUNC is_supported_mysql_version_at_least(const GrtVersionRef &mysql_version, int major,
                                                                       int minor, int release = -1);

  bool WBPUBLICBACKEND_PUBLIC_FUNC is_supported_mysql_version(int mysql_major, int mysql_minor, int mysql_release);
  bool WBPUBLICBACKEND_PUBLIC_FUNC is_supported_mysql_version(const std::string &mysql_version);
};
