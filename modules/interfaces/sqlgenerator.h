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

#ifndef _SQLGENERATOR_IF_H_
#define _SQLGENERATOR_IF_H_

#include "grtpp_module_cpp.h"
#include "grts/structs.h"
#include "grts/structs.db.h"

// diff sql generation interface definition header

namespace grt {
  class DiffChange;
};

class SQLGeneratorInterfaceImpl : public grt::InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(SQLGeneratorInterfaceImpl,
                             DECLARE_INTERFACE_FUNCTION(SQLGeneratorInterfaceImpl::getTargetDBMSName),
                             DECLARE_INTERFACE_FUNCTION(SQLGeneratorInterfaceImpl::generateSQLForDifferences),
                             DECLARE_INTERFACE_FUNCTION(SQLGeneratorInterfaceImpl::generateReportForDifferences),
                             DECLARE_INTERFACE_FUNCTION(SQLGeneratorInterfaceImpl::makeCreateScriptForObject),
                             DECLARE_INTERFACE_FUNCTION(SQLGeneratorInterfaceImpl::makeSQLExportScript),
                             DECLARE_INTERFACE_FUNCTION(SQLGeneratorInterfaceImpl::makeSQLSyncScript));

  virtual std::string getTargetDBMSName() = 0;

  // For internal use only, atm
  virtual ssize_t generateSQL(grt::Ref<GrtNamedObject>, const grt::DictRef& options,
                              std::shared_ptr<grt::DiffChange>) = 0;
  virtual grt::StringRef generateReport(grt::Ref<GrtNamedObject> org_object, const grt::DictRef& options,
                                        std::shared_ptr<grt::DiffChange>) = 0;

  virtual grt::DictRef generateSQLForDifferences(grt::Ref<GrtNamedObject>, grt::Ref<GrtNamedObject>,
                                                 grt::DictRef options) = 0;
  virtual grt::StringRef generateReportForDifferences(grt::Ref<GrtNamedObject> org_object,
                                                      grt::Ref<GrtNamedObject> oth_object,
                                                      const grt::DictRef& options) = 0;
  virtual ssize_t makeSQLExportScript(grt::Ref<GrtNamedObject>, grt::DictRef options,
                                      const grt::DictRef& objectCreateSQL, const grt::DictRef& objectDropSQL) = 0;
  virtual ssize_t makeSQLSyncScript(db_CatalogRef cat, grt::DictRef options, const grt::StringListRef& sql_list,
                                    const grt::ListRef<GrtNamedObject>& obj_list) = 0;
  virtual std::string makeCreateScriptForObject(GrtNamedObjectRef object) = 0;
  virtual grt::DictRef getDefaultTraits() const = 0;
  virtual grt::DictRef getTraitsForServerVersion(const int major, const int minor, const int revision) = 0;
};

#endif /* _SQLGENERATOR_IF_H_ */
