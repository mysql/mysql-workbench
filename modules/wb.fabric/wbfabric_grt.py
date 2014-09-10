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
from grt import log_warning, log_info
import mforms
import traceback
import time
import os
import ConfigParser
import tempfile
from workbench.os_utils import OSUtils
import StringIO
import json

from wb_common import OperationCancelledError

fabric_unavailable_message = ""
fabric_client = ""

ModuleInfo = DefineModule(name= "WBFabric", author= "Oracle Corp.", version="1.0")


def execute(command):
    output = StringIO.StringIO()

    ret_code = OSUtils.exec_command(command, output.write)

    out = output.getvalue().strip()

    return ret_code, out


def find_fabric_client():
    if os.name == 'nt':
        command = 'where mysqlfabric'
    else:
        command = 'which mysqlfabric'

    ret, out = execute(command)

    if ret:
        global fabric_unavailable_message
        fabric_unavailable_message = "MySQL Fabric support not available.\nPlease make sure MySQL Utilities version 1.4.3 or higher is installed to use this feature.\nSee log file for more details."
        log_warning("WBFabric Module", "Unable to locate the mysqlfabric utility, the fabric functions will not be available. \n"
                                       "Please make sure MySQL Utilities version 1.4.3 or higher is installed. \n"
                                       "The mysqlfabric utility path should be added into the PATH environment variable: %s\n" % out)
    else:
        global fabric_client
        fabric_client = out
        log_info("WBFabric Module", "MySQL Fabric Client found at: %s\n" % fabric_client)


def create_config(conn):
    # Retrieves the fabric node connection data
    host = conn.parameterValues["hostName"]
    port = conn.parameterValues["port"]
    user = conn.parameterValues["userName"]

    accepted, password = mforms.Utilities.find_or_ask_for_password("Fabric Node Connection", '%s@%s' % (host, port), user, False)
    if accepted:
        doc = ConfigParser.ConfigParser()

        section = 'protocol.xmlrpc'
        doc.add_section(section)
        doc.set(section, 'address', '%s:%s' % (host, port))
        doc.set(section, 'user', user)
        doc.set(section, 'password', password)
        doc.set(section, 'realm', 'MySQL Fabric')

        file = tempfile.NamedTemporaryFile(delete=False)
        file.write("# Fabric connection settings for MySQL Workbench\n\n")
        doc.write(file)

        return file.name
    else:
        raise OperationCancelledError("Password input cancelled")


def parse_standard_output(output):
    lines = output.split('\n')
    dict = {}
    for line in lines:
        if line.find('=') != -1:
            att, val = line.split('=', 2)
            if att.startswith('{'):
                att = att[1:]

            dict[att.strip()] = val.strip()

    return dict


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def testConnection(conn):

    error = fabric_unavailable_message

    if not error:
        config = ''
        try:
            config = create_config(conn)

            command = '"%s" --config="%s" manage ping' % (fabric_client, config)

            ret, out = execute(command)

            if ret:
                error = "Unexpected error while connecting to the fabric node: %s" % out
                log_warning("WBFabric Module", "Unexpected error while connecting to the fabric node: %s\n" % out)
            else:
                data = parse_standard_output(out)

                if not data.has_key('success') or data['success'] != 'True':
                    error = "Unexpected error while connecting to fabric node: %s" % out
                    log_warning("WBFabric Module", "Unexpected error while connecting to the fabric node: %s\n" % out)

        except OperationCancelledError, e:
            error = "Operation Cancelled"
            log_warning("WBFabric Module", "User cancelled testing MySQL Fabric connection %s\n" % conn.name)
        except Exception, e:
            error = str(e)
            log_warning("WBFabric Module", "Error testing MySQL Fabric connection %s : %s\n" % (conn.name, traceback.format_exc()))
        finally:
            if os.path.exists(config):
                os.remove(config)

    return error


def execute_formatted_command(config, fabric_command):
    return_data = None

    command = '"%s" --config="%s" %s' % (fabric_client, config, fabric_command)

    ret, out = execute(command)

    if ret:
        raise Exception("Unexpected error on fabric operation: %s, %s\n" % (fabric_command, out))
    else:
        formatted_data = parse_standard_output(out)

        if not formatted_data.has_key('success') or formatted_data['success'] != 'True':
            raise Exception("Unexpected error on fabric operation: %s, %s" % (fabric_command, out))
        else:
            # Swaps the quoting so the format is OK for the json loader
            json_formatted = formatted_data['return'].replace("'", "`").replace('"', "'").replace('`', '"')
            json_formatted = json_formatted.replace('False', 'false')
            json_formatted = json_formatted.replace('True', 'true')
            return_data = json.loads(json_formatted)

    return return_data


def get_managed_connections(conn):
    connections = {}

    for connection in grt.root.wb.rdbmsMgmt.storedConns:
        params = connection.parameterValues

        if params.has_key('fabric_managed') and params['fabric_managed'] == conn.__id__:
            mparams = connection.parameterValues
            address = "%s:%s" % (mparams['hostName'], mparams['port'])
            connections[address] = connection

    return connections


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def updateConnections(conn):

    error = fabric_unavailable_message

    if not error:
        config = ''

        try:
            # Creates the configuration object from the connection
            # settings.
            config = create_config(conn)

            # Pulls the HA groups
            groups = execute_formatted_command(config, 'group lookup_groups')

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
            existing_connections = get_managed_connections(conn)

            for group in groups:
                include_group = not group_filter or group['group_id'] in group_filter

                if include_group:
                    matched_groups.append(group['group_id'])

                    servers = execute_formatted_command(config, 'group lookup_servers %s' % group['group_id'])

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

        except OperationCancelledError, e:
            error = "Operation Cancelled"
            log_warning("WBFabric Module", "User cancelled creating connectios to managed servers in the %s fabric node\n" % conn.name)
        except Exception, e:
            error = str(e)
            log_warning("WBFabric Module", "Error creating connectios to managed servers in the %s fabric node : %s\n" % (conn.name, traceback.format_exc()))
        finally:
            if os.path.exists(config):
                os.remove(config)

    return error



find_fabric_client()    