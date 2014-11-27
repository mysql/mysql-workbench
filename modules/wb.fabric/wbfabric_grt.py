# Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

from wb import DefineModule
import grt
from grt import log_warning
import mforms
import traceback
import time
import json

from wb_common import OperationCancelledError

fabric_unavailable_message = ""
fabric_client = ""

ModuleInfo = DefineModule(name= "WBFabric", author= "Oracle Corp.", version="1.0")


def perform_fabric_operation(conn, name, callback = None, callback_params = None):
    """
    Current fabric operations are done using the next cycle:
    - Open Connection
    - Execute Specific Operation
    - Close Connection

    This method allows performing these operations using this life cycle, removing from the 
    specific operation the burden of common operations such as password retrieval and exception 
    handling.

    The specific operation should be done on a function received as callback.

    To pass data from the caller to the actual fabric operation method use the
    callback_params, this method will also include the connection_id on such params.
    """
    error = ""

    # Retrieves the fabric node connection data
    host = conn.parameterValues["hostName"]
    port = conn.parameterValues["port"]
    user = conn.parameterValues["userName"]

    try:
        # Retrieves the required password
        accepted, password = mforms.Utilities.find_or_ask_for_password("Fabric Node Connection", '%s@%s' % (host, port), user, False)
        if accepted:

            # Opens a connection to the fabric instance
            conn_id = grt.modules.WbFabricInterface.openConnection(conn, password)

            if conn_id > 0:
                # Executes the callback function which will interact with fabric using the
                # created connection.
                if callback:
                    if callback_params:
                        callback_params['conn_id'] = conn_id
                    else:
                        callback_params = {'conn_id':conn_id}

                    error = callback(callback_params)

                # Finally closes the connection
                grt.modules.WbFabricInterface.closeConnection(conn_id)
    except OperationCancelledError, e:
        error = "Operation Cancelled"
        log_warning("WBFabric Module", "User cancelled %s\n" % name)
    except Exception, e:
        error = str(e)
        log_warning("WBFabric Module", "Error %s : %s\n" % (conn.name, traceback.format_exc()))

    return error


def _execute_fabric_command(conn_id, fabric_command):
    """
    This function will be used to actually execute a valid fabric command
    and process the result.

    The data resulting from fabric operations is returned in JSON format.

    The Fabric commands will return 2 recordsets which are returned as 2 lists on the
    returned json data:
    - The first element is a status record, it is processe here and if there were errors
      an exception is thrown.
    - The second is the actual list of data returned by the executed function.
    """
    return_data = None

    out = grt.modules.WbFabricInterface.execute(conn_id, fabric_command)

    all_data = json.loads(out)

    status = all_data[0][0]

    if status['message']:
        raise Exception(status['message'])
    elif len(all_data) == 2:
        return_data = all_data[1]

    return return_data



def _get_managed_connections(conn):
    connections = {}

    for connection in grt.root.wb.rdbmsMgmt.storedConns:
        params = connection.parameterValues

        if params.has_key('fabric_managed') and params['fabric_managed'] == conn.__id__:
            mparams = connection.parameterValues
            address = "%s:%s" % (mparams['hostName'], mparams['port'])
            connections[address] = connection

    return connections


def _update_fabric_connections(params):

    error = ''
    conn = params['conn']
    conn_id = params['conn_id']

    # Pulls the HA groups
    groups = _execute_fabric_command(conn_id, 'call group.lookup_groups()')

    # Variables for error handling
    fabric_group_count = 0
    matched_groups = []
    added_servers = 0
    managed_connections = 0

    # Sorts the groups
    def group_key(item):
        return item['group_id']

    groups = sorted(groups, key=group_key)

    fabric_group_count = len(groups)

    group_filter = conn.parameterValues["haGroupFilter"].strip()

    # Retrieves the list of the existing managed connections
    existing_connections = _get_managed_connections(conn)

    for group in groups:
        include_group = not group_filter or group['group_id'] in group_filter

        if include_group:
            matched_groups.append(group['group_id'])

            servers = _execute_fabric_command(conn_id, 'call group.lookup_servers("%s")' % group['group_id'])

            # Sorts the servers
            def server_key(item):
                return item['address']

            servers = sorted(servers, key=server_key)

            # Creates a connection for each retrieved server.
            for server in servers:
                address = server['address']
                host, port = address.split(':')

                # If the managed servers are located on the fabric node
                # most probably they will use localhost or 127.0.0.1 as
                # address on the fabric configuration.

                # We need to replace that for the fabric node IP in order
                # to create the connections in WB
                if host in ['localhost', '127.0.0.1']:
                    host = conn.parameterValues["hostName"]

                address = '%s:%s' % (host, port)

                managed_connections += 1

                if existing_connections.has_key(address):
                    del existing_connections[address]
                else:
                    child_conn_name = '%s/%s:%s' % (conn.name, host, port)

                    server_user = conn.parameterValues["mysqlUserName"]
                    managed_conn = grt.modules.Workbench.create_connection(host, server_user, '', 1, 0, int(port), child_conn_name)
                    managed_conn.parameterValues["fabric_managed"] = conn.__id__
                    managed_conn.parameterValues["fabric_group_id"] = group["group_id"]

                    # Includes the rest of the server parameters on the connection parameters
                    for att in server.keys():
                        if att != 'address':
                            managed_conn.parameterValues['fabric_%s' % att] = server[att]

                    added_servers += 1

    # Removes the remaining connections (which no longer exist on the fabric node)
    for connection in existing_connections.values():
        grt.modules.Workbench.deleteConnection(connection)

    conn.parameterValues["managedConnectionsUpdateTime"] = time.strftime('%Y-%m-%d %H:%M:%S')

    if added_servers or existing_connections:
        grt.modules.Workbench.refreshHomeConnections()
    elif managed_connections == 0:
        if fabric_group_count == 0:
            error = "There are no High Availability Groups defined on the %s fabric node." % conn.name
        elif not matched_groups:
            error = "There are no High Availability Groups matching the configured group filter on %s." % conn.name
        else:
            error = "There are no Managed Servers defined for the included groups in %s: %s." % (conn.name, ','.join(matched_groups))

    return error
   


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def testConnection(conn):
    """
    Attempts a connection to the fabric server, returns an error if required.

    Since no additional operations are needed on such connection the specific operation callback is None.
    """
    error = perform_fabric_operation(conn, 'testing Fabric Connection')

    return error


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def updateConnections(conn):
    """
    Connects to the fabric server and updates the fabric connections in WB
    """
    error = perform_fabric_operation(conn, 'updating Fabric Servers', _update_fabric_connections, {'conn':conn} )

    return error

