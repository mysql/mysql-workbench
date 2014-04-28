/* 
 * Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_SCHEMA_EDITOR_H_
#define _MYSQL_SCHEMA_EDITOR_H_

#include "grtdb/editor_schema.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_manager.h"

#include "mysql_support_backend_public_interface.h"

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLSchemaEditorBE : public bec::SchemaEditorBE
{
  std::string _initial_name;
public:
  void refactor_catalog_upon_schema_rename(const std::string &old_name, const std::string &new_name);
  bool refactor_possible();
  void refactor_catalog();

  MySQLSchemaEditorBE(bec::GRTManager *grtm, const db_SchemaRef &schema, const db_mgmt_RdbmsRef &rdbms);
};

#endif /* _MYSQL_SCHEMA_EDITOR_H_ */
