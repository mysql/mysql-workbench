/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "wbpublic_public_interface.h"

#include "grts/structs.db.mgmt.h"
#include <string>

typedef grt::ListRef<db_CharacterSet> GrtCharacterSetsRef;

namespace bec {
  std::string WBPUBLICBACKEND_PUBLIC_FUNC get_host_identifier_for_connection(const db_mgmt_ConnectionRef &connection);
  std::string WBPUBLICBACKEND_PUBLIC_FUNC get_description_for_connection(const db_mgmt_ConnectionRef &conn);

  std::string WBPUBLICBACKEND_PUBLIC_FUNC sanitize_server_version_number(const std::string &version);

  GrtVersionRef WBPUBLICBACKEND_PUBLIC_FUNC parse_version(const std::string &version);
  int WBPUBLICBACKEND_PUBLIC_FUNC version_to_int(const GrtVersionRef &version);
  GrtVersionRef WBPUBLICBACKEND_PUBLIC_FUNC int_to_version(int version);
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
