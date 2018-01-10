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

#ifndef _DB_MYSQL_SQL_SYNC_H_
#define _DB_MYSQL_SQL_SYNC_H_

#include "db_mysql_public_interface.h"
#include "db_plugin_be.h"
#include "grts/structs.db.mysql.h"
#include "../../db.mysql/backend/db_mysql_validation_page.h"

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DbMySQLSync : public Db_plugin, public DbMySQLValidationPage {
private:
  // options
  std::string _input_filename;
  std::string _output_filename;
  std::string _script_to_apply;

public:
  DbMySQLSync();
  void set_option(const std::string& name, const std::string& value);
  void start_apply_script_to_db();
};

#endif // _DB_MYSQL_SQL_SYNC_H_
