/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_DB_PUBLIC_INTERFACE_H_
#define _MYSQL_DB_PUBLIC_INTERFACE_H_

#ifdef _WIN32

#ifdef WBPLUGINDBMYSQLBE_EXPORTS
#define WBPLUGINDBMYSQLBE_PUBLIC_FUNC __declspec(dllexport)
#else
#define WBPLUGINDBMYSQLBE_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define WBPLUGINDBMYSQLBE_PUBLIC_FUNC
#endif

#include <string>
#include <map>

#include "grts/structs.db.mysql.h"

typedef std::map<std::string, GrtNamedObjectRef> CatalogMap;

WBPLUGINDBMYSQLBE_PUBLIC_FUNC void update_all_old_names(db_mysql_CatalogRef cat, bool update_only_empty,
                                                        CatalogMap& map);
WBPLUGINDBMYSQLBE_PUBLIC_FUNC void build_catalog_map(db_mysql_CatalogRef catalog, CatalogMap& map);

#endif // _MYSQL_DB_PUBLIC_INTERFACE_H_
