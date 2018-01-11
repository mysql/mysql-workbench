/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "db_mysql_diff_reporting_public_interface.h"
#include "grt/grt_manager.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"

class DBMYSQLDIFFREPORTINGWBPBE_PUBLIC_FUNC DbMySQLDiffReportingException : public std::logic_error {
public:
  DbMySQLDiffReportingException(const std::string& message) : std::logic_error(message) {
  }
};

class DBMYSQLDIFFREPORTINGWBPBE_PUBLIC_FUNC DbMySQLDiffReporting {
public:
  inline db_mysql_CatalogRef get_model_catalog() {
    return db_mysql_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
  }

public:
  DbMySQLDiffReporting();
  virtual ~DbMySQLDiffReporting();

  std::string generate_report(const db_mysql_CatalogRef& left_cat, const db_mysql_CatalogRef& right_cat);
};
