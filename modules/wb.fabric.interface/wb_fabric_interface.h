/*
* Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grtpp_module_cpp.h"
#include "wb_fabric_interface_public_interface.h"
#include <map>
#include <mysql.h>
#include "grts/structs.db.mgmt.h"

#define WB_FABRIC_INTERFACE_VERSION "1.0"

class WB_FABRIC_INTERFACE_WBM_PUBLIC_FUNC WbFabricInterfaceImpl:
  public grt::ModuleImplBase
{
public:
  WbFabricInterfaceImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr), _connection_id(0) {};

  DEFINE_INIT_MODULE(WB_FABRIC_INTERFACE_VERSION, "MySQL AB", grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION(WbFabricInterfaceImpl::openConnection),
    DECLARE_MODULE_FUNCTION(WbFabricInterfaceImpl::closeConnection),
    DECLARE_MODULE_FUNCTION(WbFabricInterfaceImpl::execute)
  );

  int openConnection(const db_mgmt_ConnectionRef &info, const grt::StringRef &password);
  std::string execute(int connection_id, const std::string& query);
  int closeConnection(int connection_id);

private:
  int _connection_id;
  std::map<int, MYSQL> _connections;
};
