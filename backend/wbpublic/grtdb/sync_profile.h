/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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
