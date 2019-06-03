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

#include "db_helpers.h"
#include "base/string_utilities.h"
#include "grtpp_util.h"
#include "base/log.h"

using namespace base;

//----------------------------------------------------------------------------------------------------------------------

std::string bec::get_host_identifier_for_connection(const db_mgmt_ConnectionRef &connection) {
  grt::DictRef params(connection->parameterValues());
  std::string host_id;

  if (connection->driver().is_valid()) {
    std::string host_identifier = *connection->driver()->hostIdentifierTemplate();
    for (grt::DictRef::const_iterator par = params.begin(); par != params.end(); ++par) {
      base::replaceStringInplace(host_identifier, "%" + par->first + "%", par->second.toString());
    }
    return host_identifier;
  } else
    return connection->name();
}

//----------------------------------------------------------------------------------------------------------------------

std::string bec::get_description_for_connection(const db_mgmt_ConnectionRef &connection) {
  std::string conn_type;
  std::string driver, server;
  grt::DictRef params(connection->parameterValues());

  if (connection->driver().is_valid()) {
    driver = connection->driver()->name();
    server = db_mgmt_RdbmsRef::cast_from(connection->driver()->owner())->caption();
  } else
    return "Invalid Connection Description";

  std::string user = params.get_string("userName");

  if (g_str_has_suffix(driver.c_str(), "Socket")) {
    std::string path = base::trim(params.get_string("socket"));
    conn_type = base::strfmt("%s using local socket/pipe at \"%s\" with user %s", server.c_str(),
                             path.empty() ? "default path" : path.c_str(), user.c_str());
  } else if (g_str_has_suffix(driver.c_str(), "SSH")) {
    conn_type =
      base::strfmt("%s at %s:%i through SSH tunnel at %s@%s with user %s", server.c_str(),
                   params.get_string("hostName").c_str(), (int)params.get_int("port"),
                   params.get_string("sshUserName").c_str(), params.get_string("sshHost").c_str(), user.c_str());
  } else { // TCP
    conn_type = base::strfmt("%s at %s:%i with user %s", server.c_str(), params.get_string("hostName").c_str(),
                             (int)params.get_int("port"), user.c_str());
  }

  return conn_type;
}

//----------------------------------------------------------------------------------------------------------------------

std::string bec::sanitize_server_version_number(const std::string &version) {
  int major, minor, release, patch;
  if (sscanf(version.c_str(), "%i.%i.%i-%i", &major, &minor, &release, &patch) == 4) {
    return base::strfmt("%i.%i.%i-%i", major, minor, release, patch);
  } else if (sscanf(version.c_str(), "%i.%i.%i", &major, &minor, &release) == 3) {
    return base::strfmt("%i.%i.%i", major, minor, release);
  }
  return version;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Parses the given version string into its components and returns a GRT version class.
 * Unspecified components are set to -1 to allow for fuzzy comparisons.
 */
GrtVersionRef bec::parse_version(const std::string &target_version) {
  int major = 0, minor = -1, release = -1, build = -1;

  sscanf(target_version.c_str(), "%i.%i.%i.%i", &major, &minor, &release, &build);

  GrtVersionRef version(grt::Initialized);
  version->name("Version");
  version->majorNumber(major);
  version->minorNumber(minor);
  version->releaseNumber(release);
  version->buildNumber(build);

  return version;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts a grt version struct into a plain long usable by parsers.
 * Returns a default version number if the given version is invalid or has no major version.
 */
int bec::version_to_int(const GrtVersionRef &version) {
  if (!version.is_valid() || version->majorNumber() == -1)
    return 80000;

  size_t result = version->majorNumber() * 10000;
  if (version->minorNumber() > -1)
    result += version->minorNumber() * 100;
  if (version->releaseNumber() > -1)
    result += version->releaseNumber();

  return (int)result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts a grt version struct into one of the version enums.
 */
MySQLVersion bec::versionToEnum(const GrtVersionRef &version) {
  if (!version.is_valid() || version->majorNumber() == -1)
    return MySQLVersion::Unknown;

  if (version->majorNumber() >= 8)
    return MySQLVersion::MySQL80;

  if (version->majorNumber() != 5)
    return MySQLVersion::Unknown;

  switch (version->minorNumber()) {
    case 6:
      return MySQLVersion::MySQL56;

    case 7:
      return MySQLVersion::MySQL57;

    default:
      return MySQLVersion::Unknown;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts the int form of a server version to a grt version ref.
 * The build member in the returned version is always -1.
 */
GrtVersionRef bec::intToVersion(int version) {
  int major = version / 10000, minor = (version / 100) % 100, release = version % 100, build = -1;

  GrtVersionRef version_(grt::Initialized);
  version_->name("Version");
  version_->majorNumber(major);
  version_->minorNumber(minor);
  version_->releaseNumber(release);
  version_->buildNumber(build);

  return version_;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Compares the given version numbers to see if a is equal to b.
 * In order to support wildcards, missing values are interpreted as matching, e.g. 5 is equal to 5.1
 * and 5.1 is equal to 5.1.1.
 *
 * Do not use for comparing supported MySQL server versions.
 */
bool bec::version_equal(GrtVersionRef a, GrtVersionRef b) {
  // Major version number is always there.
  if (a->majorNumber() != b->majorNumber())
    return false;

  if (a->minorNumber() == -1 || b->minorNumber() == -1)
    return true;

  if (a->minorNumber() != b->minorNumber())
    return false;

  if (a->releaseNumber() == -1 || b->releaseNumber() == -1)
    return true;

  if (a->releaseNumber() != b->releaseNumber())
    return false;

  if (a->buildNumber() == -1 || b->buildNumber() == -1)
    return true;

  if (a->buildNumber() != b->buildNumber())
    return false;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This test is similar to the one above (except that it tests for greater-than relations).
 * For this however we have to treat unset values differently to handle cases like
 * 5 > 5.1 (false) or 5.1 > 5 (true) correctly.
 *
 * Do not use for comparing supported MySQL server versions.
 */
bool bec::version_greater(GrtVersionRef a, GrtVersionRef b) {
  if (a->majorNumber() > b->majorNumber()) // Major number should always be set.
    return true;

  if (a->majorNumber() == b->majorNumber()) {
    if (a->minorNumber() == -1) // An unset value can never be larger than anything else.
      return false;

    if (b->minorNumber() == -1) // An unset value is always smaller than any other set value.
      return true;

    if (a->minorNumber() > b->minorNumber())
      return true;

    // Same approach for release + build numbers.
    if (a->minorNumber() == b->minorNumber()) {
      if (a->releaseNumber() == -1)
        return false;

      if (b->releaseNumber() == -1)
        return true;

      if (a->releaseNumber() > b->releaseNumber())
        return true;

      if (a->releaseNumber() == b->releaseNumber()) {
        if (a->buildNumber() == -1)
          return false;

        if (b->buildNumber() == -1)
          return true;

        if (a->buildNumber() > b->buildNumber())
          return true;
      }
      return false;
    }
    return false;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/** Checks if the given server version numbers is in the set of supported MySQL servers
 */
bool bec::is_supported_mysql_version(int mysql_major, int mysql_minor, int mysql_release) {
  return ((mysql_major == 5 && (mysql_minor == 6 || mysql_minor == 7)) ||
          (mysql_major == 8 && mysql_minor == 0));
}

//----------------------------------------------------------------------------------------------------------------------

bool bec::is_supported_mysql_version(const std::string &mysql_version) {
  int my_major = 0, my_minor = -1, my_release = -1, my_build = -1;

  sscanf(mysql_version.c_str(), "%i.%i.%i.%i", &my_major, &my_minor, &my_release, &my_build);

  return is_supported_mysql_version(my_major, my_minor, my_release);
}

//----------------------------------------------------------------------------------------------------------------------

/** Checks whether the version number supplied is in the known set of versions larger than it.
 Use for server version checks for features.
 */
bool bec::is_supported_mysql_version_at_least(int mysql_major, int mysql_minor, int mysql_release, int major, int minor,
                                              int release) {
  // if the version required is older (<) than 5.6, then any server that matches is fine
  // if the version required is newer (>=) than 5.6, then we can only guarantee that known servers versions have the
  // feature

  assert(mysql_major < 100 && mysql_minor < 100 && mysql_release < 1000);
  assert(major < 100 && minor < 100 && release < 1000);

  // assemble MMNNRRR
  unsigned int required = major * 100000 + minor * 1000 + (release < 0 ? 0 : release);
  // if the available release number is negative, that's meant to signify "any release number", so we make it the max
  // value possible
  unsigned int available = mysql_major * 100000 + mysql_minor * 1000 + (mysql_release < 0 ? 999 : mysql_release);

  if (major < 5 || (major == 5 && minor < 6) || (major == 8 && minor == 0)) {
    return (required <= available);
  } else if (is_supported_mysql_version(mysql_major, mysql_minor, mysql_release)) {
    return (required <= available);
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool bec::is_supported_mysql_version_at_least(const std::string &mysql_version, int major, int minor, int release) {
  int my_major = 0, my_minor = -1, my_release = -1, my_build = -1;

  sscanf(mysql_version.c_str(), "%i.%i.%i.%i", &my_major, &my_minor, &my_release, &my_build);

  return is_supported_mysql_version_at_least(my_major, my_minor, my_release, major, minor, release);
}

//----------------------------------------------------------------------------------------------------------------------

bool bec::is_supported_mysql_version_at_least(const GrtVersionRef &mysql_version, int major, int minor, int release) {
  if (mysql_version.is_valid())
    return is_supported_mysql_version_at_least((int)mysql_version->majorNumber(), (int)mysql_version->minorNumber(),
                                               (int)mysql_version->releaseNumber(), major, minor, release);
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
