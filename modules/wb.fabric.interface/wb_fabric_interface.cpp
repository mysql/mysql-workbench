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

#include "wb_fabric_interface.h"
#include "base/string_utilities.h"

#include <stdexcept>


GRT_MODULE_ENTRY_POINT(WbFabricInterfaceImpl);

int WbFabricInterfaceImpl::openConnection(const db_mgmt_ConnectionRef &conn, const grt::StringRef &password)
{
  int new_connection_id = -1;

  MYSQL mysql;

  mysql_init(&mysql);

  // Retrieves the parameters needed to perform the connection
  std::string user = conn->parameterValues().get_string("userName");
  std::string host = conn->parameterValues().get_string("hostName");
  std::string socket = conn->parameterValues().get_string("socket");
  int port = (int)conn->parameterValues().get_int("port");

  // If the port is not specified it will take the default port to perform
  // fabric connections using mysqlrpc.
  if (port <= 0)
    port = 32275;

  // Sets the options needed to connect to the fabric server.
  int proto = MYSQL_PROTOCOL_TCP;
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &proto);
  
  
  if (!mysql_real_connect(&mysql, host.c_str(), user.c_str(), password.c_str(), NULL, port, socket.c_str(),
    CLIENT_COMPRESS | CLIENT_MULTI_RESULTS))
  {
    throw std::runtime_error(mysql_error(&mysql));
  }

  // A new connection_id is created and will be returned, the actuall connection
  // will be stored here and used through the generated id.
  new_connection_id = ++_connection_id;
  _connections[new_connection_id] = mysql;


  // Changes the format of the execution results to JSON
  // This saves the burden on having to parse the output in C code
  // and allows taking advantage of the JSON loader in python
  execute(new_connection_id, "set format=json");

  return new_connection_id;
}

std::string WbFabricInterfaceImpl::execute(int connection_id, const std::string& query)
{
  int error = 0;
  std::string output;
  if (_connections.find(connection_id) != _connections.end())
  {
    error = mysql_query(&_connections[connection_id], query.c_str());

    if (!error)
    {
      MYSQL_RES *result = mysql_store_result(&_connections[connection_id]);

      // Since output is created in JSON format, a single string record
      // contains all the needed data
      if (result)
      {
        MYSQL_ROW row = mysql_fetch_row(result);

        output = row[0];

        mysql_free_result(result);
      }

      // If there are no results we need to check that it is correct to not
      // have any, else it means an error occurred
      else
        error = mysql_field_count(&_connections[connection_id]);
    }
    if (error != 0)
    {
      // Escape strings to create valid JSON structure
      std::string formatted(query);
      size_t index = 0;
      do
      {
        index = formatted.find("\"", index);
        if (index != std::string::npos)
        {
          formatted.insert(index, "\\");
          index += 2;
        }
      } while (index != std::string::npos);

      output = base::strfmt("[[{\"message\":\"SQL Error executing %s: %d - %s\"}]]", formatted.c_str(), mysql_errno(&_connections[connection_id]), mysql_error(&_connections[connection_id]));
    }
  }
  else
    output = "[[{\"message\":\"Invalid Connection Id\"}]]";

  return output;
}

int WbFabricInterfaceImpl::closeConnection(int connection_id)
{
  if (_connections.find(connection_id) != _connections.end())
  {
    mysql_close(&_connections[connection_id]);

    _connections.erase(connection_id);
  }

  return 0;
}




