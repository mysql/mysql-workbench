/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"
#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.workbench.physical.h"
#include "wbpublic_public_interface.h"

namespace bec {
  WBPUBLICBACKEND_PUBLIC_FUNC db_mgmt_SyncProfileRef create_sync_profile(workbench_physical_ModelRef model,
                                                                         const std::string &profile_name,
                                                                         const std::string &target_schema);
  WBPUBLICBACKEND_PUBLIC_FUNC db_mgmt_SyncProfileRef get_sync_profile(workbench_physical_ModelRef model,
                                                                      const std::string &profile_name,
                                                                      const std::string &target_schema);
  WBPUBLICBACKEND_PUBLIC_FUNC void update_schema_from_sync_profile(db_SchemaRef schema, db_mgmt_SyncProfileRef profile);
  WBPUBLICBACKEND_PUBLIC_FUNC void update_sync_profile_from_schema(db_mgmt_SyncProfileRef profile, db_SchemaRef schema,
                                                                   bool view_code_only = false);
};
