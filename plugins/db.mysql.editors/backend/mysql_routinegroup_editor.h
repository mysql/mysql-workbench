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

#ifndef _MYSQL_ROUTINE_GROUP_EDITOR_H_
#define _MYSQL_ROUTINE_GROUP_EDITOR_H_

#include "grtdb/editor_routinegroup.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.mysql.h"

#include "mysql_support_backend_public_interface.h"

class MYSQLWBMYSQLSUPPORTBACKEND_PUBLIC_FUNC MySQLRoutineGroupEditorBE : public bec::RoutineGroupEditorBE
{
public:
  MySQLRoutineGroupEditorBE(bec::GRTManager *grtm, const db_mysql_RoutineGroupRef &group, const db_mgmt_RdbmsRef &rdbms);

  std::string get_procedure_body();
  std::string get_function_body();
  
  void load_routines_sql();
  virtual void commit_changes();

};

#endif /* _MYSQL_ROUTINE_GROUP_EDITOR_H_ */
