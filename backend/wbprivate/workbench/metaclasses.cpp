/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

// code to auto-register all metaclasses/structs used in Workbench

#include "grts/structs.app.h"
#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.migration.h"
#include "grts/structs.db.mssql.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.db.query.h"
#include "grts/structs.db.sybase.h"
#include "grts/structs.eer.h"
#include "grts/structs.h"
#include "grts/structs.meta.h"
#include "grts/structs.model.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.logical.h"
#include "grts/structs.workbench.model.h"
#include "grts/structs.workbench.model.reporting.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.ui.h"
#include "grts/structs.wrapper.h"
#include "wb_backend_public_interface.h"

void register_all_metaclasses() {
  register_structs_app_xml();
  register_structs_db_xml();
  register_structs_db_mgmt_xml();
  register_structs_db_migration_xml();
  register_structs_db_mssql_xml();
  register_structs_db_mysql_xml();
  // register_structs_db_oracle_xml();
  register_structs_db_query_xml();
  register_structs_db_sybase_xml();
  register_structs_eer_xml();
  register_structs_xml();
  register_structs_meta_xml();
  register_structs_model_xml();
  register_structs_workbench_xml();
  register_structs_workbench_logical_xml();
  register_structs_workbench_model_xml();
  register_structs_workbench_model_reporting_xml();
  register_structs_workbench_physical_xml();
  register_structs_ui_xml();
  register_structs_wrapper_xml();
}
