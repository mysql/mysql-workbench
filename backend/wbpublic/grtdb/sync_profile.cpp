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

#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "sync_profile.h"
#include "grtpp_util.h"
#include "grt/common.h"

db_mgmt_SyncProfileRef bec::create_sync_profile(workbench_physical_ModelRef model, const std::string &profile_name,
                                                const std::string &target_schema) {
  db_mgmt_SyncProfileRef profile(grt::Initialized);
  profile->targetHostIdentifier(profile_name);
  profile->targetSchemaName(grt::StringRef(target_schema));

  model->syncProfiles().set(
    base::strfmt("%s::%s", profile->targetHostIdentifier().c_str(), profile->targetSchemaName().c_str()), profile);

  return profile;
}

db_mgmt_SyncProfileRef bec::get_sync_profile(workbench_physical_ModelRef model, const std::string &profile_name,
                                             const std::string &target_schema) {
  return db_mgmt_SyncProfileRef::cast_from(
    model->syncProfiles().get(base::strfmt("%s::%s", profile_name.c_str(), target_schema.c_str())));
}

/** update_schema_from_sync_profile()

 Updates synchronization related info in the schema from synchronization profile object.
 */
void bec::update_schema_from_sync_profile(db_SchemaRef schema, db_mgmt_SyncProfileRef profile) {
  grt::DictRef lastKnownDBNames(profile->lastKnownDBNames());

  schema->oldName(grt::StringRef::cast_from(lastKnownDBNames.get(schema.id(), schema->oldName())));

  GRTLIST_FOREACH(db_Table, schema->tables(), table) {
    (*table)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*table).id(), (*table)->oldName())));

    GRTLIST_FOREACH(db_Column, (*table)->columns(), column) {
      (*column)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*column).id(), (*column)->oldName())));
    }
    GRTLIST_FOREACH(db_Index, (*table)->indices(), index) {
      (*index)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*index).id(), (*index)->oldName())));
    }
    GRTLIST_FOREACH(db_ForeignKey, (*table)->foreignKeys(), fk) {
      (*fk)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*fk).id(), (*fk)->oldName())));
    }
    GRTLIST_FOREACH(db_Trigger, (*table)->triggers(), trigger) {
      (*trigger)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*trigger).id(), (*trigger)->oldName())));
    }
  }
  GRTLIST_FOREACH(db_View, schema->views(), view) {
    (*view)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*view).id(), (*view)->oldName())));
    (*view)->oldModelSqlDefinition(grt::StringRef::cast_from(
      profile->lastKnownViewDefinitions().get((*view).id() + ":model", (*view)->oldModelSqlDefinition())));
    (*view)->oldServerSqlDefinition(grt::StringRef::cast_from(
      profile->lastKnownViewDefinitions().get((*view).id() + ":server", (*view)->oldServerSqlDefinition())));
  }
  GRTLIST_FOREACH(db_Routine, schema->routines(), routine) {
    (*routine)->oldName(grt::StringRef::cast_from(lastKnownDBNames.get((*routine).id(), (*routine)->oldName())));
  }
}

/** update_sync_profile_from_schema()

 Updates the synchronization profile object with info from the schema.

 The Synchronization Profile stores the following info from schemas, on a per target server
 and target schema name basis:
 - oldNames from all objects, so that we can remember how is an object named at the server end,
   for renaming detection purposes. This is stored in lastKnownDBNames
 - the model and server version of SQL DDL for views, so that we can detect view changes in either
   the server or the model. Stored in lastKnownViewDefinitions
 */
void bec::update_sync_profile_from_schema(db_mgmt_SyncProfileRef profile, db_SchemaRef schema, bool view_code_only) {
  grt::DictRef lastKnownDBNames(profile->lastKnownDBNames());
  if (!view_code_only)
    lastKnownDBNames.reset_entries();
  profile->lastKnownViewDefinitions().reset_entries();

  if (!view_code_only) {
    lastKnownDBNames.set(schema.id(), schema->name());
    GRTLIST_FOREACH(db_Table, schema->tables(), table) {
      lastKnownDBNames.set((*table).id(), (*table)->name());

      GRTLIST_FOREACH(db_Column, (*table)->columns(), column) {
        lastKnownDBNames.set((*column).id(), (*column)->name());
      }
      GRTLIST_FOREACH(db_Index, (*table)->indices(), index) {
        lastKnownDBNames.set((*index).id(), (*index)->name());
      }
      GRTLIST_FOREACH(db_ForeignKey, (*table)->foreignKeys(), fk) {
        lastKnownDBNames.set((*fk).id(), (*fk)->name());
      }
      GRTLIST_FOREACH(db_Trigger, (*table)->triggers(), trigger) {
        lastKnownDBNames.set((*trigger).id(), (*trigger)->name());
      }
    }
  }
  GRTLIST_FOREACH(db_View, schema->views(), view) {
    lastKnownDBNames.set((*view).id(), (*view)->name());
    profile->lastKnownViewDefinitions().set((*view).id() + ":model", (*view)->oldModelSqlDefinition());
    profile->lastKnownViewDefinitions().set((*view).id() + ":server", (*view)->oldServerSqlDefinition());
  }
  if (!view_code_only) {
    GRTLIST_FOREACH(db_Routine, schema->routines(), routine) {
      lastKnownDBNames.set((*routine).id(), (*routine)->name());
    }
    profile->lastSyncDate(base::fmttime(0, "%Y-%m-%d %H:%M:%S"));
  }
}
