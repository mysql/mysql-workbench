/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "base/session_wrapper.h"

#include "shellcore/types.h"
#include "shellcore/shell_registry.h"
#include "utils/utils_file.h"
#include "modules/mysqlxtest_utils.h"
#include "base/string_utilities.h"

dataTypes::ProjectHolder ng::loadNgSessions()
{
  shcore::Server_registry sr(shcore::get_default_config_path());

  try {
    sr.load();
  } catch (...)
  {
    translate_crud_exception("ShellRegistry.load");
  }

  dataTypes::ProjectHolder holder;
  for (std::map<std::string, shcore::Connection_options>::const_iterator it = sr.begin(); it != sr.end(); ++it)
  {
    const shcore::Connection_options& cs = it->second;

    dataTypes::ProjectHolder entry;
    entry.name = cs.get_name();
    entry.project.name = cs.get_name();
    entry.project.connection.hostName = cs.get_server();
    entry.project.connection.userName = cs.get_user();
    entry.project.connection.port = base::atoi<int>(cs.get_port());
    entry.project.connection.uuid = cs.get_uuid();

    holder.children.push_back(entry);
  }

//  dataTypes::ProjectHolder holder;
//  for (auto item : *(shcore::StoredSessions::get_instance()->connections()))
//  {
//    dataTypes::ProjectHolder entry;
//    shcore::Value::Map_type_ref connection = item.second.as_map();
//    entry.name = item.first;
//    entry.project.name = item.first;
//    entry.project.connection.hostName = (*connection)["host"].as_string();
//    entry.project.connection.userName = (*connection)["dbUser"].as_string();
//    entry.project.connection.port = (*connection)["port"].as_int();
//    (*connection).
//    holder.children.push_back(entry);
//  }

  return holder;
}

bool ng::storeNgSession(const dataTypes::XProject &project)
{
  return shcore::StoredSessions::get_instance()->add_connection(project.name, project.connection.uri(false));
}

bool ng::deleteNgSession(const dataTypes::XProject &project)
{
  return shcore::StoredSessions::get_instance()->remove_connection(project.name);
}

dataTypes::XProject ng::getSessionByUUID(const std::string &uuid)
{
  shcore::Server_registry sr(shcore::get_default_config_path());

  try {
    sr.load();
  } catch (...)
  {
    translate_crud_exception("ShellRegistry.load");
  }

  dataTypes::XProject project;
  for (std::map<std::string, shcore::Connection_options>::const_iterator it = sr.begin(); it != sr.end(); ++it)
  {
    const shcore::Connection_options& cs = it->second;
    if (cs.get_uuid() == uuid)
    {
      project.name = cs.get_name();
      project.connection.hostName = cs.get_server();
      project.connection.userName = cs.get_user();
      project.connection.port = base::atoi<int>(cs.get_port());
      project.connection.uuid = cs.get_uuid();
      break;
    }
  }
  return project;
}
