/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <stack>

#include <glib.h>
#include <boost/signals2.hpp>

#include "mysql_sql_facade.h"

using namespace bec;
using namespace grt;

#define MODULE_VERSION "1.0.0"

GRT_MODULE_ENTRY_POINT(MysqlSqlFacadeImpl);

/*
grt::ListRef<app_Plugin> MysqlSqlFacadeImpl::getPluginInfo()
{
  grt::ListRef<app_Plugin> list(true);
  return list;
}
*/
