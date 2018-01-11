/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtpp_module_cpp.h"

#include "grts/structs.db.mgmt.h"
#include "grt/spatial_handler.h"
#include "base/log.h"

#define Utilities_VERSION "1.0.0"

DEFAULT_LOG_DOMAIN("utilities");

class UtilitiesImpl : public grt::ModuleImplBase {
public:
  UtilitiesImpl(grt::CPPModuleLoader *loader) : grt::ModuleImplBase(loader) {
  }

  DEFINE_INIT_MODULE(Utilities_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(UtilitiesImpl::loadRdbmsInfo),
                     DECLARE_MODULE_FUNCTION_DOC(UtilitiesImpl::fetchAuthorityCodeFromWKT,
                                                 "Parse WKT SRS string and extract EPSG code from it.",
                                                 "wkt is an SRS string that contains WellKnownText data."),
                     DECLARE_MODULE_FUNCTION_DOC(UtilitiesImpl::fetchAuthorityCodeFromFile,
                                                 "Load WKT SRS from file and extract EPSG code from it.",
                                                 "path the path to file that contains SRS WKT."),
                     NULL);

  db_mgmt_RdbmsRef loadRdbmsInfo(db_mgmt_ManagementRef owner, const std::string &path) {
    db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize(path));

    rdbms->owner(owner);

    return rdbms;
  }

  std::string fetchAuthorityCodeFromWKT(const std::string &wkt) {
    return spatial::fetchAuthorityCode(wkt);
  }

  std::string fetchAuthorityCodeFromFile(const std::string &path) {
    gchar *data;
    gsize length;
    std::string epsg;
    if (g_file_get_contents(path.c_str(), &data, &length, NULL)) {
      epsg = spatial::fetchAuthorityCode(data);
      g_free(data);
    } else
      logError("Unable to get contents of a file: %s\n", path.c_str());
    return epsg;
  }
};

GRT_MODULE_ENTRY_POINT(UtilitiesImpl);
