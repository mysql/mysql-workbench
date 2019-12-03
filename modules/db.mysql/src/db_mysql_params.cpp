/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "db_mysql_params.h"
#include "grtpp_util.h"
#include "grt/grt_manager.h"
#include "base/file_utilities.h"

namespace dbmysql {

  grt::ListRef<db_mysql_StorageEngine> get_known_engines() {
    return grt::ListRef<db_mysql_StorageEngine>::cast_from(grt::GRT::get()->unserialize(
      base::makePath(bec::GRTManager::get()->get_basedir(), "modules/data/mysql_engines.xml")));
  }

}
